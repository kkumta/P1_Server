#pragma once
class Iocp
{
public:
	Iocp();
	~Iocp();

	HANDLE		GetHandle() { return _iocpHandle; }

	bool		Register(IocpObjectPtr iocpObject);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocpHandle;
};