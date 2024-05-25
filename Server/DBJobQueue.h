#pragma once
#include "JobQueue.h"
#include "DBJobQueue.h"

class DBJobQueue : public JobQueue
{
public:
	DBJobQueue();
	virtual ~DBJobQueue();

	bool HandleJoin(PacketSessionPtr& session, Protocol::C_JOIN& pkt);
	bool HandleLogin(PacketSessionPtr& session, Protocol::C_LOGIN& pkt);
};

extern DBJobQueuePtr GDBJobQueue;