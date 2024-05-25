#pragma once

#include <thread>
#include <functional>

/*------------------
	ThreadManager
-------------------*/

class ThreadManager
{
public:
	enum class Type
	{
		LOGIC,
		DB
	};

	ThreadManager();
	~ThreadManager();

	void	Launch(function<void(void)> callback);
	void	Join();

	static void InitTLS();
	static void DestroyTLS();

	static void DoGlobalQueueWork(THREAD_TYPE type);
	static void DistributeReservedJobs();

private:
	mutex			_lock;
	vector<thread>	_threads;
};