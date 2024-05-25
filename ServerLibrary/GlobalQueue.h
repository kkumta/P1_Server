#pragma once

class GlobalQueue
{
public:
	GlobalQueue();
	~GlobalQueue();

	void Push(JobQueuePtr jobQueue, THREAD_TYPE type);
	JobQueuePtr Pop(THREAD_TYPE type);

private:
	LockQueue<JobQueuePtr> _logicJobQueues;
	LockQueue<JobQueuePtr> _dbJobQueues;
};