#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "DBConnectionPool.h"
#include "DBJobQueue.h"
#include "Room.h"

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

		// 글로벌 큐
		ThreadManager::DoGlobalQueueWork(THREAD_TYPE::LOGIC);
	}
}

void DoDBJob()
{
	while (true)
	{
		ThreadManager::DoGlobalQueueWork(THREAD_TYPE::DB);
	}
}

int main()
{
	ServerPacketHandler::Init();

	ServerServicePtr service = make_shared<ServerService>(
		SockAddress(L"127.0.0.1", 7777),
		make_shared<Iocp>(),
		[=]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		100);

	ASSERT_CRASH(service->Start());

	// 게임 로직 스레드
	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([&service]()
		{
			DoWorkerJob(service);
		});
	}

	// DB 스레드
	ASSERT_CRASH(GDBConnectionPool->Connect(1, L"Driver={SQL Server Native Client 11.0};Server=(localdb)\\MSSQLLocalDB;Database=Phoenix;Trusted_Connection=Yes;"));
	{
		auto query = L"									\
			DROP TABLE IF EXISTS [dbo].[users];				\
			CREATE TABLE [dbo].[users]						\
			(												\
				[id] INT NOT NULL PRIMARY KEY IDENTITY,		\
				[nickname] VARCHAR(32) NOT NULL,			\
				[password] VARCHAR(32) NOT NULL				\
			);";

		DBConnection* dbConn = GDBConnectionPool->Pop();
		ASSERT_CRASH(dbConn->Execute(query));
		GDBConnectionPool->Push(dbConn);
	}
	for (int32 i = 0; i < 2; i++)
	{
		GThreadManager->Launch([=]()
		{
			DoDBJob();
		});
	}

	GThreadManager->Join();
}