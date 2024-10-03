#include "pch.h"
#include "ServerPacketHandler.h"
#include "DBJobQueue.h"
#include "Room.h"
#include "GameSession.h"
#include "Player.h"
#include "ObjectUtils.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(PacketSessionPtr& session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_C_JOIN(PacketSessionPtr& session, Protocol::C_JOIN& pkt)
{
	GDBJobQueue->DoAsync(&DBJobQueue::HandleJoin, session, pkt);

	return true;
}

bool Handle_C_LOGIN(PacketSessionPtr& session, Protocol::C_LOGIN& pkt)
{
	GDBJobQueue->DoAsync(&DBJobQueue::HandleLogin, session, pkt);

	return true;
}

bool Handle_C_ENTER_GAME(PacketSessionPtr& session, Protocol::C_ENTER_GAME& pkt)
{
	PlayerPtr player = ObjectUtils::CreatePlayer(static_pointer_cast<GameSession>(session), pkt.nickname());

	// �÷��̾ �濡 �����Ų��.
	GRoom->DoAsync(&Room::HandleEnterPlayer, player);

	return false;
}

bool Handle_C_LEAVE_GAME(PacketSessionPtr& session, Protocol::C_LEAVE_GAME& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerPtr player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomPtr room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleLeavePlayer, player);

	return true;
}

bool Handle_C_MOVE(PacketSessionPtr& session, Protocol::C_MOVE& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);

	PlayerPtr player = gameSession->player.load();
	if (player == nullptr)
		return false;

	// gameSession�� Player�� pkt�� info�� object_id�� �ش�Ǵ� Player���� Ȯ��
	if (player->posInfo->object_id() != pkt.info().object_id())
		return false;

	RoomPtr room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleMove, pkt);

	return true;
}

bool Handle_C_ATTACK(PacketSessionPtr& session, Protocol::C_ATTACK& pkt)
{
	auto gameSession = static_pointer_cast<GameSession>(session);
	
	PlayerPtr player = gameSession->player.load();
	if (player == nullptr)
		return false;

	RoomPtr room = player->room.load().lock();
	if (room == nullptr)
		return false;

	room->DoAsync(&Room::HandleAttack, pkt);

	return false;
}

bool Handle_C_CHAT(PacketSessionPtr& session, Protocol::C_CHAT& pkt)
{
	return false;
}