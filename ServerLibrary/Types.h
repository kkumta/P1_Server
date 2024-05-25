#pragma once

#include <mutex>
#include <atomic>

using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

#define USING_SHARED_PTR(name)	using name##Ptr = std::shared_ptr<class name>;

USING_SHARED_PTR(Iocp);
USING_SHARED_PTR(IocpObject);
USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);
USING_SHARED_PTR(Listener);
USING_SHARED_PTR(Service);
USING_SHARED_PTR(ServerService);
USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(Job);
USING_SHARED_PTR(JobQueue);

enum THREAD_TYPE
{
	LOGIC = 1,
	DB = 2
};