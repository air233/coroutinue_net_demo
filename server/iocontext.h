#pragma once
#include <winsock2.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

class Socket;
class AcceptAwaiter;
class RecvAwaiter;
class SendAwaiter;

class IoContext
{
public:
	IoContext() :m_Run(false) 
	{
		FD_ZERO(&m_readFds);
		FD_ZERO(&m_writeFds);
	}
	bool Start();
	void Stop();
	void Run();

protected:
	friend Socket;
	friend AcceptAwaiter;
	friend RecvAwaiter;
	friend SendAwaiter;

	void Attach(Socket* socket);
	void Detach(Socket* socket);

	void WatchRead(Socket* socket);
	void UnWatchRead(Socket* socket);

	void WatchWrite(Socket* socket);
	void UnWatchWrite(Socket* socket);

	void HandleEvents(const fd_set& readFds, const fd_set& writeFds);
	BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType);
private:
	bool m_Run;

	fd_set m_readFds;
	fd_set m_writeFds;
	std::unordered_set<Socket*> m_sockets;
};