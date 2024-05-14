#pragma once

#include "Macro.h"
#include "SockAddress.h"
#include "Iocp.h"
#include "Session.h"
#include "Listener.h"
#include <functional>

enum ServiceType
{
	CLIENT,
	SERVER,
};

using SessionFactory = function<SessionPtr(void)>;

class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType serviceType, SockAddress sockAddress, IocpPtr iocp, SessionFactory sessionFactory, uint16 maxSessionCount);
	virtual ~Service();

	virtual bool Start() abstract;
	bool IsSessionFactoryAvailable();

	virtual void Close();

	SessionPtr CreateSession();
	void AddSession(SessionPtr session);
	void ReleaseSession(SessionPtr session);
	int32 GetCurrentSessionCount() { return _curSessionCount; }
	int32 GetMaxSessionCount() { return _maxSessionCount; }

	ServiceType GetServiceType() { return _serviceType; }
	SockAddress GetSockAddress() { return _sockAddress; }
	IocpPtr& GetIocp() { return _iocp; }

protected:
	USE_LOCK;
	ServiceType _serviceType;
	SockAddress _sockAddress;
	IocpPtr _iocp;
	SessionFactory _sessionFactory;

	set<SessionPtr> _sessions;
	uint16 _maxSessionCount = 0;
	uint16 _curSessionCount = 0;
};

class ServerService : public Service
{
public:
	ServerService(SockAddress sockAddress, IocpPtr iocp, SessionFactory factory, uint16 maxSessionCount = 1);
	virtual ~ServerService() {}

	virtual bool Start() override;
	virtual void Close() override;

private:
	ListenerPtr _listener = nullptr;
};