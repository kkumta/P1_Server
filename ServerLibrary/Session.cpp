#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"

Session::Session() : _recvBuffer(BUFFER_SIZE)
{
	_socket = SocketUtils::CreateSocket();
}

Session::~Session()
{
	if (_socket != INVALID_SOCKET)
		::closesocket(_socket);

	_socket = INVALID_SOCKET;
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::Disconnect(const WCHAR* cause)
{
	if (_isConnected.exchange(false) == false)
		return;

	RegisterDisconnect();
}

void Session::Send(SendBufferPtr sendBuffer)
{
	if (IsConnected() == false)
		return;

	{
		EXCLUSIVE_LOCK;

		_sendBuffers.push(sendBuffer);

		if (!_sendRegistered.exchange(true))
			RegisterSend();
	}
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes)
{
	switch (iocpEvent->GetEventType())
	{
	case EventType::CONNECT:
		ProcessConnect();
		break;
	case EventType::DISCONNECT:
		ProcessDisconnect();
		break;
	case EventType::SEND:
		ProcessSend(numOfBytes);
		break;
	case EventType::RECV:
		ProcessRecv(numOfBytes);
		break;
	default:
		break;
	}
}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::CLIENT)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::BindAnyAddress(_socket, 0/*남는거*/) == false)
		return false;

	_connectEvent.Init();
	_connectEvent.SetOwner(shared_from_this()); // ADD Session REF

	DWORD numOfBytes = 0;
	SOCKADDR_IN sockAddr = _service.lock()->GetSockAddress().GetSockAddr();
	if (false == SocketUtils::ConnectEx(_socket, reinterpret_cast<SOCKADDR*>(&sockAddr), sizeof(sockAddr), nullptr, 0, &numOfBytes, &_connectEvent))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_connectEvent.SetOwner(nullptr); // RELEASE Session REF
			return false;
		}
	}

	return true;
}

void Session::ProcessConnect()
{
	_connectEvent.SetOwner(nullptr); // Release Session Ref
	_isConnected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionPtr());

	// 컨텐츠 코드에서 재정의
	OnConnected();

	// 수신 등록
	RegisterRecv();
}

bool Session::RegisterDisconnect()
{
	_disconnectEvent.Init();
	_disconnectEvent.SetOwner(shared_from_this()); // Add Session Ref

	if (false == SocketUtils::DisconnectEx(_socket, &_disconnectEvent, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			_disconnectEvent.SetOwner(nullptr); // Release Session Ref
			return false;
		}
	}

	return true;
}

void Session::ProcessDisconnect()
{
	_disconnectEvent.SetOwner(nullptr); // Release Session Ref

	OnDisconnected(); // 컨텐츠 코드에서 재정의
	GetService()->ReleaseSession(GetSessionPtr());
}

void Session::RegisterSend()
{
	if (!IsConnected())
		return;

	_sendEvent.Init();
	_sendEvent.SetOwner(shared_from_this());

	// _sendBuffers -> _sendEvent.sendBuffers
	{
		int32 writeSize = 0;
		while (_sendBuffers.empty() == false)
		{
			SendBufferPtr sendBuffer = _sendBuffers.front();

			writeSize += sendBuffer->WriteSize();
			// TODO : 예외 체크

			_sendBuffers.pop();
			_sendEvent.sendBuffers.push_back(sendBuffer);
		}
	}

	// _sendEvent.sendBuffers -> wsaBufs
	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(_sendEvent.sendBuffers.size());
	for (SendBufferPtr sendBuffer : _sendEvent.sendBuffers)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = reinterpret_cast<char*>(sendBuffer->Buffer());
		wsaBuf.len = static_cast<LONG>(sendBuffer->WriteSize());
		wsaBufs.push_back(wsaBuf);
	}

	DWORD numOfBytes = 0;
	if (::WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &_sendEvent, nullptr) == SOCKET_ERROR)
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_sendEvent.SetOwner(nullptr); // Session RELEASE_REF
			_sendEvent.sendBuffers.clear(); // SendBuffer RELEASE_REF
			_sendRegistered.store(false);
		}
	}
}

// _sendBuffers가 비어있거나, 중간에 _sendBuffers에 새로운 데이터가 추가될 경우 호출
void Session::ProcessSend(int32 numOfBytes)
{
	_sendEvent.SetOwner(nullptr); // Session RELEASE_REF
	_sendEvent.sendBuffers.clear(); // SendBuffer RELEASE_REF

	if (numOfBytes == 0)
	{
		Disconnect(L"Send 0");
		return;
	}

	// 컨텐츠 코드에서 재정의
	OnSend(numOfBytes);

	EXCLUSIVE_LOCK;

	if (_sendBuffers.empty())
		_sendRegistered.store(false);
	else
		RegisterSend();
}

void Session::RegisterRecv()
{
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.SetOwner(shared_from_this()); // Add Session Ref

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer.WritePos());
	wsaBuf.len = _recvBuffer.FreeSize();

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, OUT & flags, &_recvEvent, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			HandleError(errorCode);
			_recvEvent.SetOwner(nullptr); // Release Session Ref
		}
	}
}

void Session::ProcessRecv(int32 numOfBytes)
{
}

void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

PacketSession::PacketSession()
{
}

PacketSession::~PacketSession()
{
}

// [size(2)][id(2)][data....][size(2)][id(2)][data....]
int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
		if (dataSize < header.size)
			break;

		// 패킷 조립 성공
		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}
