#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "JobTimer.h"

/*--------------
	JobQueue
---------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	explicit JobQueue(THREAD_TYPE threadType) : _threadType(threadType) {}

	void DoAsync(CallbackType&& callback, THREAD_TYPE type)
	{
		Push(make_shared<Job>(std::move(callback)), type);
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());

		// owner가 DBJobQueue 클래스 타입이면 실행
		if (owner.get()->_threadType == THREAD_TYPE::DB)
			Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...), THREAD_TYPE::DB);
		// owner가 그 외 클래스 타입이면 실행
		else
			Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...), THREAD_TYPE::LOGIC);
	}

	// DoTimer의 스레드 타입은 무조건 LOGIC이다
	void DoTimer(uint64 tickAfter, CallbackType&& callback)
	{
		JobPtr job = make_shared<Job>(std::move(callback));
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	template<typename T, typename Ret, typename... Args>
	void DoTimer(uint64 tickAfter, Ret(T::* memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		JobPtr job = make_shared<Job>(owner, memFunc, std::forward<Args>(args)...);
		GJobTimer->Reserve(tickAfter, shared_from_this(), job);
	}

	void ClearJobs() { _jobs.Clear(); }

public:
	void Push(JobPtr job, THREAD_TYPE type, bool pushOnly = false);
	void Execute(THREAD_TYPE type);

protected:
	LockQueue<JobPtr> _jobs;
	atomic<int32> _jobCount = 0;
	THREAD_TYPE _threadType;
};