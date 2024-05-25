#pragma once

#include "Macro.h"
#include "Iocp.h"
#include "IocpObject.h"
#include "IocpEvent.h"
#include "SockAddress.h"
#include "RecvBuffer.h"

class Service;

class Session : public IocpObject
{
	friend class Listener;

	enum
	{
		BUFFER_SIZE = 0x10000,
	};

private:
	USE_LOCK;

public:
	Session();
	virtual ~Session();

	// 다른 클래스에서 사용하는 함수
	bool Connect();
	void Disconnect(const WCHAR* cause);
	void Send(SendBufferPtr sendBuffer);

	// Get, Set
	ServicePtr GetService() { return _service.lock(); }
	void SetService(ServicePtr service) { _service = service; }

	SOCKET GetSocket() { return _socket; }
	SessionPtr GetSessionPtr() { return static_pointer_cast<Session>(shared_from_this()); }
	bool IsConnected() { return _isConnected; }

private:
	// IocpObject의 함수 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0);

	// Iocp Register, Process
	bool RegisterConnect();
	void ProcessConnect();

	bool RegisterDisconnect();
	void ProcessDisconnect();

	void RegisterSend();
	void ProcessSend(int32 numOfBytes); // 실제로 처리할 데이터가 있는 경우에만 호출

	void RegisterRecv();
	void ProcessRecv(int32 numOfBytes);

	void HandleError(int32 errorCode);

protected:
	// Session을 상속받는 클래스에서 구현할 함수
	virtual void OnConnected() {}
	virtual void OnDisconnected() {}
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 len) {}

private:
	// 연결 관련 변수
	weak_ptr<Service> _service;
	SOCKET _socket = INVALID_SOCKET;
	atomic<bool> _isConnected = false;

	// 송수신 관련 변수
	queue<SendBufferPtr> _sendBuffers;
	atomic<bool> _sendRegistered = false;
	RecvBuffer _recvBuffer;

	// IocpEvent 관련 변수
	ConnectEvent _connectEvent;
	DisconnectEvent _disconnectEvent;
	RecvEvent _recvEvent;
	SendEvent _sendEvent;
};

struct PacketHeader
{
	uint16 size;
	uint16 id;
};

class PacketSession : public Session
{
public:
	PacketSession();
	virtual ~PacketSession();

	PacketSessionPtr GetPacketSessionPtr() { return static_pointer_cast<PacketSession>(shared_from_this()); }

protected:
	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
};