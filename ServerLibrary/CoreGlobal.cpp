#include "pch.h"
#include "CoreGlobal.h"
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "SendBuffer.h"
#include "GlobalQueue.h"
#include "JobTimer.h"
#include "DBConnectionPool.h"

ThreadManager* GThreadManager = nullptr;

GlobalQueue* GGlobalQueue = nullptr;
JobTimer* GJobTimer = nullptr;

DBConnectionPool* GDBConnectionPool = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GThreadManager = new ThreadManager();
		GGlobalQueue = new GlobalQueue();
		GJobTimer = new JobTimer();

		GDBConnectionPool = new DBConnectionPool();

		SocketUtils::Init();
	}

	~CoreGlobal()
	{
		delete GThreadManager;
		delete GGlobalQueue;
		delete GJobTimer;

		delete GDBConnectionPool;

		SocketUtils::Clear();
	}
} GCoreGlobal;