#pragma once

class SockAddress
{
public:
	SockAddress() = default;
	SockAddress(SOCKADDR_IN sockAddr);
	SockAddress(wstring ip, uint16 port);

	SOCKADDR_IN& GetSockAddr() { return _sockAddr; }

private:
	static IN_ADDR	Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN _sockAddr = {};
};