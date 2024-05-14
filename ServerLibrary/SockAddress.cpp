#include "pch.h"
#include "SockAddress.h"

SockAddress::SockAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr)
{
}

SockAddress::SockAddress(wstring ip, uint16 port)
{
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = Ip2Address(ip.c_str());
	_sockAddr.sin_port = ::htons(port);
}

IN_ADDR SockAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}