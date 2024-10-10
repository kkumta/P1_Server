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

	// ���ο� �÷��̾� ����
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

	// ���ο� ��ü�� �����ߴٴ� ����� �ٸ� �÷��̾�� �˸���
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->objectInfo);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->objectInfo->object_id());
	}

	// ������ �����ϴ� ��ü ����� ���ο� �÷��̾����� �����Ѵ�
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

	// ���� ����� �����ϴ� �÷��̾�� �˸���
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_LEAVE_GAME leaveGamePkt;

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(leaveGamePkt);
		if (auto session = player->session.lock())
			session->Send(sendBuffer);
	}

	// ���� ����� �˸���
	{
		Protocol::S_DESPAWN despawnPkt;
		despawnPkt.add_object_ids(objectId);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(despawnPkt);
		Broadcast(sendBuffer, objectId);

		if (auto player = dynamic_pointer_cast<Player>(object))
			if (auto session = player->session.lock())
				session->Send(sendBuffer);
	}

	// ���Ͱ� �׾��� ��� �������Ѵ�
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
	if (success) // �÷��̾ ���������� �������� ��� ���� ����
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

	// ����
	PlayerPtr player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());
	GLogger.logMove(CreatureType_Name(player->objectInfo->creature_type()), player->objectInfo->object_id(), player->objectInfo->nickname(), player->posInfo->x(), player->posInfo->y(), player->posInfo->z(), player->posInfo->yaw(), true);

	// ��� Ŭ���̾�Ʈ���� �̵� ����� �˸���
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

	// ���� �� �ǰ�
	CreaturePtr attackerCreature = dynamic_pointer_cast<Creature>(_objects[attackerId]);
	CreaturePtr victimCreature = dynamic_pointer_cast<Creature>(_objects[victimId]);

	GLogger.logAttack(CreatureType_Name(attackerCreature->objectInfo->creature_type()), attackerId, attackerCreature->objectInfo->nickname(), CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), attackerCreature->creatureInfo->damage(), attackerCreature->posInfo->x(), attackerCreature->posInfo->y(), attackerCreature->posInfo->z(), attackerCreature->posInfo->yaw(), true);

	// �ǰݴ��� Creature�� HP�� 0 ���ϰ� �Ǿ� �Ҹ��ϴ� ���
	if (victimCreature->creatureInfo->cur_hp() <= attackerCreature->creatureInfo->damage())
	{
		GLogger.logHit(CreatureType_Name(attackerCreature->objectInfo->creature_type()), attackerId, attackerCreature->objectInfo->nickname(), CreatureType_Name(victimCreature->objectInfo->creature_type()), victimId, victimCreature->objectInfo->nickname(), 0, attackerCreature->creatureInfo->damage(), attackerCreature->posInfo->x(), attackerCreature->posInfo->y(), attackerCreature->posInfo->z(), attackerCreature->posInfo->yaw(), true);

		// �Ҹ��ϴ� Creature�� ������ ���, Player�� EXP�� ������Ų��
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

		// ��� Ŭ���̾�Ʈ���� ���� �� �ǰ� ����� �˸���
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
		// ���Ͱ� ������ createCount * 0.1�� �� ����
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
	// �̹� �ش� object_id�� �߰��� ��ü�� �ִٸ�, ���� ó��
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
	{
		GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), false, "object_id already exist");
		return false;
	}
	// �̹� �ش� nickname���� ������ �÷��̾ ������, ���� ó��
	if (PlayerPtr player = dynamic_pointer_cast<Player>(object))
	{
		if (_players.find(object->objectInfo->nickname()) != _players.end())
		{
			GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), false, "nickname already exist");
			return false;
		}

		// ���ٸ�, �߰��Ѵ�
		_players.insert(make_pair(object->objectInfo->nickname(), object->objectInfo->object_id()));
	}

	_objects.insert(make_pair(object->objectInfo->object_id(), object));
	object->room.store(GetRoomPtr());

	GLogger.logSpawn(CreatureType_Name(object->objectInfo->creature_type()), object->objectInfo->object_id(), object->objectInfo->nickname(), object->posInfo->x(), object->posInfo->y(), object->posInfo->z(), object->posInfo->yaw(), true);
	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// ���ٸ� ������ �ִ�.
	if (_objects.find(objectId) == _objects.end())
		return false;

	ObjectPtr object = _objects[objectId];

	// ������ ��ü�� �÷��̾��� ���
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