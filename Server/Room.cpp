#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "Monster.h"
#include "GameSession.h"
#include "ObjectUtils.h"
#include "DBJobQueue.h"
#include "GameData.h"
#include "Logger.h"

RoomPtr GRoom = make_shared<Room>();

Room::Room() : JobQueue(THREAD_TYPE::LOGIC)
{
	for (uint64 i = 0; i < TOTAL_MONSTER_COUNT; i++)
		_monsters.push_back(make_pair(false, i));
}

Room::~Room()
{
}

bool Room::EnterRoom(ObjectPtr object)
{
	bool success = AddObject(object);
	if (!success)
		return false;

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
		DoTimer(MONSTER_RESPAWN_DELAY, &Room::UpdateMonster);
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
	{
		GLogger.logMove("", 0, "", pkt.info().x(), pkt.info().y(), pkt.info().z(), pkt.info().yaw(), false, "object_id not exist");
		return;
	}

	// 적용
	PlayerPtr player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());
	GLogger.logMove(CreatureType_Name(player->objectInfo->creature_type()), player->objectInfo->object_id(), player->objectInfo->nickname(), player->posInfo->x(), player->posInfo->y(), player->posInfo->z(), player->posInfo->yaw(), true);

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
	const uint64 attackerId = pkt.attacking_object_id();
	const uint64 victimId = pkt.attacked_object_id();
	if (_objects.find(attackerId) == _objects.end() || _objects.find(victimId) == _objects.end())
	{
		GLogger.logAttack("", attackerId, "", "", victimId, "", 0, 0, 0, 0, 0, false, "attackerId/victimId not exist");
		return;
	}

	// 공격 및 피격
	CreaturePtr attackerCreature = dynamic_pointer_cast<Creature>(_objects[attackerId]);
	CreaturePtr victimCreature = dynamic_pointer_cast<Creature>(_objects[victimId]);

	GLogger.logAttack(CreatureType_Name(attackerCreature->objectInfo->creature_type()), attackerId, attackerCreature->objectInfo->nickname(), CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), attackerCreature->creatureInfo->damage(), attackerCreature->posInfo->x(), attackerCreature->posInfo->y(), attackerCreature->posInfo->z(), attackerCreature->posInfo->yaw(), true);

	// 피격당한 Creature의 HP가 0 이하가 되어 소멸하는 경우
	if (victimCreature->creatureInfo->cur_hp() <= attackerCreature->creatureInfo->damage())
	{
		GLogger.logHit(CreatureType_Name(attackerCreature->objectInfo->creature_type()), attackerId, attackerCreature->objectInfo->nickname(), CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), 0, attackerCreature->creatureInfo->damage(), attackerCreature->posInfo->x(), attackerCreature->posInfo->y(), attackerCreature->posInfo->z(), attackerCreature->posInfo->yaw(), true);

		// 소멸하는 Creature가 몬스터일 경우, Player의 EXP를 증가시킨다
		if (auto victimMonster = dynamic_pointer_cast<Monster>(victimCreature))
		{
			const uint64 rewardExp = GGameData->GetMonster(victimMonster->monsterInfo->monster_number()).GetRewardExp();
			GDBJobQueue->DoAsync(&DBJobQueue::HandleIncreaseExp, attackerCreature->objectInfo->nickname(), rewardExp);
		}
		LeaveRoom(victimCreature);
	}
	else
	{
		uint64 newHp = victimCreature->creatureInfo->cur_hp() - attackerCreature->creatureInfo->damage();
		victimCreature->creatureInfo->set_cur_hp(newHp);

		GLogger.logHit(CreatureType_Name(attackerCreature->objectInfo->creature_type()), attackerId, attackerCreature->objectInfo->nickname(), CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), victimCreature->creatureInfo->cur_hp(), attackerCreature->creatureInfo->damage(), attackerCreature->posInfo->x(), attackerCreature->posInfo->y(), attackerCreature->posInfo->z(), attackerCreature->posInfo->yaw(), true);
		GLogger.logDeath(CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), true);

		// 모든 클라이언트에게 공격 및 피격 사실을 알린다
		{
			Protocol::S_ATTACK attackPkt;
			{
				Protocol::CreatureInfo* info = attackPkt.mutable_info();
				info->CopyFrom(*(victimCreature->creatureInfo));
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
		// 몬스터가 없으면 createCount * 0.1초 후 생성
		if (_monsters[i].first == false)
		{
			createCount++;
			_monsters[i].first = true;
			MonsterPtr monster = ObjectUtils::CreateMonster(_monsters[i].second);
			DoTimer(createCount * MONSTER_SPAWN_DELAY, &Room::HandleEnterMonster, monster);
		}
	}
}

RoomPtr Room::GetRoomPtr()
{
	return static_pointer_cast<Room>(shared_from_this());
}

bool Room::AddObject(ObjectPtr object)
{
	// 이미 해당 object_id로 추가된 객체가 있다면, 실패 처리
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
	{
		GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), false, "object_id already exist");
		return false;
	}
	// 이미 해당 nickname으로 접속한 플레이어가 있으면, 실패 처리
	if (PlayerPtr player = dynamic_pointer_cast<Player>(object))
	{
		if (_players.find(object->objectInfo->nickname()) != _players.end())
		{
			GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), false, "nickname already exist");
			return false;
		}

		// 없다면, 추가한다
		_players.insert(make_pair(object->objectInfo->nickname(), object->objectInfo->object_id()));
	}

	_objects.insert(make_pair(object->objectInfo->object_id(), object));
	object->room.store(GetRoomPtr());

	GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), true);
	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// 없다면 문제가 있다.
	if (_objects.find(objectId) == _objects.end())
		return false;

	ObjectPtr object = _objects[objectId];

	// 삭제될 객체가 플레이어일 경우
	PlayerPtr player = dynamic_pointer_cast<Player>(object);
	if (player)
	{
		player->room.store(weak_ptr<Room>());
		_players.erase(player->objectInfo->nickname());
	}

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