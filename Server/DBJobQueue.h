#pragma once

class DBJobQueue : public JobQueue
{
public:
	DBJobQueue();
	virtual ~DBJobQueue();

	bool HandleJoin(PacketSessionPtr session, Protocol::C_JOIN pkt);
	bool HandleLogin(PacketSessionPtr session, Protocol::C_LOGIN pkt);
	void HandleIncreaseExp(const string nickname, const uint64 rewardExp);
};

extern DBJobQueuePtr GDBJobQueue;