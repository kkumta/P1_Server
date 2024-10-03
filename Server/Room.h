#pragma once
#include "JobQueue.h"

class Room : public JobQueue
{
public:
	Room();
	virtual ~Room();

	bool HandleEnterPlayer(PlayerPtr player);
	bool HandleLeavePlayer(PlayerPtr player);

	bool HandleEnterMonster(MonsterPtr monster);

	// Obejct�� Room���� �ϴ� ���� ����
	bool EnterRoom(ObjectPtr object);
	bool LeaveRoom(ObjectPtr object);
	void HandleMove(Protocol::C_MOVE pkt);
	void HandleAttack(Protocol::C_ATTACK pkt);

	// ������ ���� �ֱ⸶�� ���� ���� ����
	void UpdateMonster();

	RoomPtr GetRoomPtr();

private:
	bool AddObject(ObjectPtr object);
	bool RemoveObject(uint64 objectId);

private:
	void Broadcast(SendBufferPtr sendBuffer, uint64 exceptId = 0);

private:

	enum
	{
		TOTAL_MONSTER_COUNT = 4,
	};

	unordered_map<uint64, ObjectPtr> _objects;
	vector<pair<bool, uint64>> _monsters;
};

extern RoomPtr GRoom;