#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include "Monster.h"
#include "ObjectUtils.h"

RoomPtr GRoom = make_shared<Room>();

Room::Room()
{

}

Room::~Room()
{

}

bool Room::EnterRoom(ObjectPtr object)
{
	bool success = AddObject(object);

	// �⺻ ��ġ ����
	object->posInfo->set_x(100.f);
	object->posInfo->set_y(100.f);
	object->posInfo->set_z(100.f);
	object->posInfo->set_yaw(50.f);

	// ���� ����� ���� �÷��̾�� �˸���
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

	// ���� ����� �ٸ� �÷��̾�� �˸���
	{
		Protocol::S_SPAWN spawnPkt;

		Protocol::ObjectInfo* objectInfo = spawnPkt.add_objects();
		objectInfo->CopyFrom(*object->objectInfo);

		SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(spawnPkt);
		Broadcast(sendBuffer, object->objectInfo->object_id());
	}

	// ���� ������ �÷��̾� ����� ���� �÷��̾����� �������ش�
	if (auto player = dynamic_pointer_cast<Player>(object))
	{
		Protocol::S_SPAWN spawnPkt;

		for (auto& item : _objects)
		{
			if (item.second->IsPlayer() == false)
				continue;

			Protocol::ObjectInfo* playerInfo = spawnPkt.add_objects();
			playerInfo->CopyFrom(*item.second->objectInfo);
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

	return success;
}

bool Room::HandleEnterPlayer(PlayerPtr player)
{
	return EnterRoom(player);
}

bool Room::HandleLeavePlayer(PlayerPtr player)
{
	return LeaveRoom(player);
}

void Room::HandleMove(Protocol::C_MOVE pkt)
{
	const uint64 objectId = pkt.info().object_id();
	if (_objects.find(objectId) == _objects.end())
		return;

	// ����
	PlayerPtr player = dynamic_pointer_cast<Player>(_objects[objectId]);
	player->posInfo->CopyFrom(pkt.info());

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

void Room::UpdateTickMonster()
{
}

void Room::UpdateTick()
{
	cout << "Update Room" << endl;

	DoTimer(100, &Room::UpdateTick);
}

RoomPtr Room::GetRoomPtr()
{
	return static_pointer_cast<Room>(shared_from_this());
}

bool Room::AddObject(ObjectPtr object)
{
	// �ִٸ� ������ �ִ�.
	if (_objects.find(object->objectInfo->object_id()) != _objects.end())
		return false;

	_objects.insert(make_pair(object->objectInfo->object_id(), object));

	object->room.store(GetRoomPtr());

	return true;
}

bool Room::RemoveObject(uint64 objectId)
{
	// ���ٸ� ������ �ִ�.
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