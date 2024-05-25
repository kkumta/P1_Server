#include "pch.h"
#include "ThreadManager.h"
#include "TLS.h"
#include "CoreGlobal.h"
#include "GlobalQueue.h"

/*------------------
	ThreadManager
-------------------*/

ThreadManager::ThreadManager()
{
	// Main Thread
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	lock_guard<mutex> guard(_lock);

	_threads.push_back(thread([=]()
	{
		InitTLS();
		callback();
		DestroyTLS();
	}));
}

void ThreadManager::Join()
{
	for (thread& t : _threads)
	{
		if (t.joinable())
			t.join();
	}
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	static atomic<uint32> SThreadId = 1;
	LThreadId = SThreadId.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}

void ThreadManager::DoGlobalQueueWork(THREAD_TYPE type)
{
	while (true)
	{
		uint64 now = ::GetTickCount64();
		if (now > LEndTickCount)
			break;

		JobQueuePtr jobQueue = GGlobalQueue->Pop(type);
		if (jobQueue == nullptr)
			break;

		jobQueue->Execute(type);
	}
}

void ThreadManager::DistributeReservedJobs()
{
	const uint64 now = ::GetTickCount64();

	GJobTimer->Distribute(now);
}