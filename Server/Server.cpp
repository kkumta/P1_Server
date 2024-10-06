#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "DBConnectionPool.h"
#include "DBJobQueue.h"
#include "Room.h"
#include "GameData.h"

enum
{
	WORKER_TICK = 64
};

void DoWorkerJob(ServerServicePtr& service)
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 네트워크 입출력 처리 -> 인게임 로직까지 (패킷 핸들러에 의해)
		service->GetIocp()->Dispatch(10);

		// 예약된 일감 처리
		ThreadManager::DistributeReservedJobs();

		// 글로벌 큐에 있는 잡큐 일감 처리
		ThreadManager::DoGlobalQueueWork(THREAD_TYPE::LOGIC);
	}
}

void DoDBJob()
{
	while (true)
	{
		LEndTickCount = ::GetTickCount64() + WORKER_TICK;

		// 글로벌 큐에 있는 잡큐 일감 처리
		ThreadManager::DoGlobalQueueWork(THREAD_TYPE::DB);
	}
}

int main()
{
	// Load GameData(Json File)
	GGameData->LoadData("../Common/GameData.json");

	// DB Connect
	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={SQL Server Native Client 11.0};Server=(localdb)\\MSSQLLocalDB;Database=Phoenix;Trusted_Connection=Yes;"));

	// Create users table
	{
		auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[users];				\
			CREATE TABLE [dbo].[users]						\
			(												\
				[id] INT NOT NULL PRIMARY KEY IDENTITY,		\
				[nickname] VARCHAR(32) NOT NULL,			\
				[password] VARCHAR(32) NOT NULL,			\
				[exp] BIGINT DEFAULT 0 NOT NULL				\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}

	// Init ServerPacketHandler
	ServerPacketHandler::Init();

	// Start ServerService
	ServerServicePtr service = make_shared<ServerService>(
		SockAddress(L"127.0.0.1", 7777),
		make_shared<Iocp>(),
		[=]() { return make_shared<GameSession>(); },
		100);

	ASSERT_CRASH(service->Start());

	// Start GameLogicThread
	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([&service]()
		{
			DoWorkerJob(service);
		});
	}

	// Start DB Thread
	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([]()
		{
			DoDBJob();
		});
	}

	GThreadManager->Join();
}