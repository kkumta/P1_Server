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

	// Obejct가 Room에서 하는 행위 관련
	bool EnterRoom(ObjectPtr object);
	bool LeaveRoom(ObjectPtr object);
	void HandleMove(Protocol::C_MOVE pkt);
	void HandleAttack(Protocol::C_ATTACK pkt);

	// 서버가 일정 주기마다 몬스터 스폰 관리
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
		MONSTER_RESPAWN_DELAY = 10000,
		MONSTER_SPAWN_DELAY = 100,
	};

	unordered_map<uint64, ObjectPtr> _objects; // uint64: object_id
	unordered_map<string, uint64> _players; // string: nickname, uint64: object_id
	vector<pair<bool, uint64>> _monsters; // uint64: monster_number
};

extern RoomPtr GRoom;