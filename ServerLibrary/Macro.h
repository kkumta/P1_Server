#pragma once

#define OUT

/* LOCK */
#define USE_MANY_LOCKS(count) mutex _locks[count];
#define USE_LOCK USE_MANY_LOCKS(1)
#define	EXCLUSIVE_LOCK_IDX(idx) lock_guard<mutex> lockGuard_##idx(_locks[idx]);
#define EXCLUSIVE_LOCK EXCLUSIVE_LOCK_IDX(0)

/* CRASH */
#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}