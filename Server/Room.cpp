#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"
#include "ObjectUtils.h"

RoomPtr GRoom = make_shared<Room>();

Room::Room()
{
	for (uint64 i = 0; i < TOTAL_MONSTER_COUNT; i++)
	{
		MonsterPtr monster = make_shared<Monster>();
		monster->posInfo->set_x((float)i * 1000);
		monster->posInfo->set_y((float)i * 500);
		monster->posInfo->set_z(88.f);
		monster->posInfo->set_yaw(20.f);
		monster->objectInfo->set_nickname("monster");
		monster->monsterInfo->set_monster_number(i);
		_monsters.push_back(make_pair(false, monster));
	}
}

Room::~Room()
{

}

bool Room::EnterRoom(ObjectPtr object)
{
	bool success = AddObject(object);
	if (!success)
		return false;

	// 플레이어일 경우 기본 위치 설정
	if (dynamic_pointer_cast<Player>(object))
	{
		object->posInfo->set_x(100.f);
		object->posInfo->set_y(100.f);
		object->posInfo->set_z(100.f);
		object->posInfo->set_yaw(50.f);
	}

	// 새로운 플레이어 입장
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_ENTER_GAME enterGamePkt;
		enterGamePkt.set_success(success);

		Protocol::ObjectInfo* playerInfo = new Protocol::ObjectInfo();
		playerInfo->CopyFrom(*object->objectInfo);
		enterGamePkt.set_allocated_player(playerInfo);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(enterGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// 새로운 객체가 입장했다는 사실을 다른 플레이어에게 알린다
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->objectInfo);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->objectInfo->object_id());
	}

	// 기존에 존재하는 객체 목록을 새로운 플레이어한테 전송한다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _objects)
		{
			Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
			objectInfo->CopyFrom(*item.second->objectInfo);
		}

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	return success;
}

bool Room::LeaveRoom(ObjectPtr object)
{
	if (object == nullptr)
		return false;

	const uint64 objectId = object->objectInfo->object_id();
	bool success = RemoveObject(objectId);

	// 퇴장 사실을 퇴장하는 플레이어에게 알린다
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// 퇴장 사실을 알린다
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto player = dynamic_pointer_cast<Player>(object))
			if (auto session = player->session.lock())
				session->Send(sendBuffer);
	}

	// 몬스터가 죽었을 경우 리스폰한다
	if (auto monster = dynamic_pointer_cast<Monster>(object))
	{
		_monsters[monster->monsterInfo->monster_number()].first = false;
		DoTimer(10000, &Room::UpdateMonster);
	}

	return success;
}

bool Room::HandleEnterPlayer(PlayerPtr player)
{
	bool success = EnterRoom(player);
	if (success) // 플레이어가 성공적으로 입장했을 경우 몬스터 생성
		UpdateMonster();

	return success;
}

bool Room::HandleLeavePlayer(PlayerPtr player)
{
	return LeaveRoom(player);
}

bool Room::HandleEnterMonster(MonsterPtr monster)
{
	return EnterRoom(monster);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_objects.find(objectId) == _objects.end())
		return;

	// 적용
	PlayerPtr player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());

	// 모든 클라이언트에게 이동 사실을 알린다
	{
		Protocol::S_MOVE movePkt;
		{
			Protocol::PosInfo* info = movePkt.mutable_info();
			info->CopyFrom(pkt.info());
		}
		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(movePkt);
		Broadcast(sendBuffer);
	}
}

void Room::HandleAttack(Protocol::C_ATTACK pkt)
{
	const uint64 attackingId = pkt.attacking_object_id();
	const uint64 attackedId = pkt.attacked_object_id();
	if (_objects.find(attackingId) == _objects.end() || _objects.find(attackedId) == _objects.end())
		return;

	// 공격 및 피격
	CreaturePtr attackingCreature = dynamic_pointer_cast<Creature>(_objects[attackingId]);
	CreaturePtr attackedCreature = dynamic_pointer_cast<Creature>(_objects[attackedId]);

	// 피격당한 Creature의 HP가 0 이하가 되어 소멸하는 경우
	if (attackedCreature->creatureInfo->cur_hp() <= attackingCreature->creatureInfo->damage())
	{
		// TODO: 소멸 및 디스폰 패킷 전송
		LeaveRoom(attackedCreature);
	}
	else
	{
		uint64 newHp = attackedCreature->creatureInfo->cur_hp() - attackingCreature->creatureInfo->damage();
		attackedCreature->creatureInfo->set_cur_hp(newHp);

		// 모든 클라이언트에게 공격 및 피격 사실을 알린다
		{
			Protocol::S_ATTACK attackPkt;
			{
				Protocol::CreatureInfo* info = attackPkt.mutable_info();
				info->CopyFrom(*(attackedCreature->creatureInfo));
			}
			SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(attackPkt);
			Broadcast(sendBuffer);
		}
	}
}

void Room::UpdateMonster()
{
	uint64 createCount = 0;
	for (int i = 0; i < _monsters.size(); i++)
	{
		if (_monsters[i].second == nullptr)
			continue;

		// 몬스터가 없으면 createCount * 0.1초 후 생성
		if (_monsters[i].first == false)
		{
			createCount++;
			_monsters[i].first = true;
			MonsterPtr monster = ObjectUtils::CreateMonster(_monsters[i].second);
			DoTimer(createCount * 100, &Room::HandleEnterMonster, monster);
		}
	}
}

// TODO
// 플레이어가 공격하는 함수
// 몬스터 체력이 0이 되어 사망하고 리스폰되는 함수->UpdateTickMonster 호출

//void Room::UpdateTick()
//{
//	cout << "Update Room" << endl;
//
//	DoTimer(100, &Room::UpdateTick);
//}

RoomPtr Room::GetRoomPtr()
{
	return static_pointer_cast<Room>(shared_from_this());
}

bool Room::AddObject(ObjectPtr object)
{
	// 있다면 문제가 있다.
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->objectInfo->object_id(), object));

	object->room.store(GetRoomPtr());

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// 없다면 문제가 있다.
	if (_objects.find(objectId) == _objects.end())
		return false;

	ObjectPtr object = _objects[objectId];
	PlayerPtr player = dynamic_pointer_cast<Player>(object);
	if (player)
		player->room.store(weak_ptr<Room>());

	_objects.erase(objectId);

	return true;
}

void Room::Broadcast(SendBufferPtr sendBuffer, uint64 exceptId)
{
	for (auto& item : _objects)
	{
		PlayerPtr player = dynamic_pointer_cast<Player>(item.second);
		if (player == nullptr)
			continue;
		if (player->objectInfo->object_id() == exceptId)
			continue;

		if (GameSessionPtr session = player->session.lock())
			session->Send(sendBuffer);
	}
}