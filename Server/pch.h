#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#ifdef _DEBUG
#pragma comment(lib, "ServerLibrary\\Debug\\ServerLibrary.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerLibrary\\Release\\ServerLibrary.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#include "CorePch.h"

#include "Protocol.pb.h"
#include "Enum.pb.h"
#include "Struct.pb.h"
#include "ServerPacketHandler.h"
#include "GameSession.h"

USING_SHARED_PTR(GameSession);
USING_SHARED_PTR(Player);
USING_SHARED_PTR(Monster);
USING_SHARED_PTR(Creature);
USING_SHARED_PTR(Object);
USING_SHARED_PTR(Room);
USING_SHARED_PTR(DBJobQueue);

#define SEND_PACKET(pkt)													\
	SendBufferPtr sendBuffer = ServerPacketHandler::MakeSendBuffer(pkt);	\
	session->Send(sendBuffer);