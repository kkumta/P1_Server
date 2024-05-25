#include "pch.h"
#include "GlobalQueue.h"

/*----------------
	GlobalQueue
-----------------*/

GlobalQueue::GlobalQueue()
{

}

GlobalQueue::~GlobalQueue()
{

}

void GlobalQueue::Push(JobQueuePtr jobQueue, THREAD_TYPE type)
{
	if (type == THREAD_TYPE::LOGIC)
		_logicJobQueues.Push(jobQueue);
	else if (type == THREAD_TYPE::DB)
		_dbJobQueues.Push(jobQueue);
}

JobQueuePtr GlobalQueue::Pop(THREAD_TYPE type)
{
	if (type == THREAD_TYPE::LOGIC)
		return _logicJobQueues.Pop();
	else if (type == THREAD_TYPE::DB)
		return _dbJobQueues.Pop();
}