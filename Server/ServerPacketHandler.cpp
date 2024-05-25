#include "pch.h"
#include "ServerPacketHandler.h"
#include "DBJobQueue.h"
#include "Room.h"
#include "GameSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionPtr& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_C_JOIN(PacketSessionPtr& session, Protocol::C_JOIN& pkt)
{
	GDBJobQueue->HandleJoin(session, pkt);

	return true;
}

bool Handle_C_LOGIN(PacketSessionPtr& session, Protocol::C_LOGIN& pkt)
{
	GDBJobQueue->HandleLogin(session, pkt);

	return false;
}

bool Handle_C_ENTER_GAME(PacketSessionPtr& session, Protocol::C_ENTER_GAME& pkt)
{
	return false;
}

bool Handle_C_LEAVE_GAME(PacketSessionPtr& session, Protocol::C_LEAVE_GAME& pkt)
{
	return false;
}

bool Handle_C_MOVE(PacketSessionPtr& session, Protocol::C_MOVE& pkt)
{
	return false;
}

bool Handle_C_CHAT(PacketSessionPtr& session, Protocol::C_CHAT& pkt)
{
	return false;
}