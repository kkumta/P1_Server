#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"

Service::Service(ServiceType serviceType, SockAddress sockAddress, IocpPtr iocp, SessionFactory sessionFactory, uint16 maxSessionCount) : _serviceType(serviceType), _sockAddress(sockAddress), _iocp(iocp), _sessionFactory(sessionFactory), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

bool Service::IsSessionFactoryAvailable()
{
	return _sessionFactory != nullptr;
}

void Service::Close()
{
}

SessionPtr Service::CreateSession()
{
	SessionPtr session = _sessionFactory();
	session->SetService(shared_from_this());

	if (_iocp->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionPtr session)
{
	EXCLUSIVE_LOCK;

	_curSessionCount++;
	_sessions.insert(session);
}

void Service::ReleaseSession(SessionPtr session)
{
	EXCLUSIVE_LOCK;

	ASSERT_CRASH(_sessions.erase(session) != 0);
	_curSessionCount--;
}

ServerService::ServerService(SockAddress sockAddress, IocpPtr iocp, SessionFactory factory, uint16 maxSessionCount) : Service(ServiceType::SERVER, sockAddress, iocp, factory, maxSessionCount)
{
}

bool ServerService::Start()
{
	if (!IsSessionFactoryAvailable())
		return false;

	_listener = make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	ServerServicePtr service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::Close()
{
	Service::Close();
}