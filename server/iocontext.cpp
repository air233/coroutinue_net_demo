#include "iocontext.h"
#include <Winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "socket.h"
#include <csignal>

// 控制台事件处理函数
BOOL WINAPI IoContext::ConsoleCtrlHandler(DWORD ctrlType)
{
	switch (ctrlType) {
	case CTRL_C_EVENT: // Ctrl+C 事件
		printf("Ctrl+C pressed. Exiting gracefully...\n");
		// 在这里可以执行额外的清理操作
		return TRUE; // 返回 TRUE 表示已处理该事件
	case CTRL_CLOSE_EVENT: // 控制台关闭事件
		printf("Console close event received. Exiting...\n");
		return TRUE;
	default:
		return FALSE; // 返回 FALSE 表示未处理该事件
	}
}

bool IoContext::Start()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
	{
		printf("winsock load faild!\n");
		return false;
	}
	m_Run = true;

	//TODO:注册ctrl+c事件 退出操作
	return true;
}


void IoContext::Stop()
{
	WSACleanup();
}

void IoContext::Run()
{
	printf("IoContext running!\n");

	while (m_Run)
	{
		fd_set readFds = m_readFds;
		fd_set writeFds = m_writeFds;

		timeval timeout{};
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000; // 10ms超时，减少CPU占用

		int result = select(0, &readFds, &writeFds, nullptr, &timeout);
		if (result == SOCKET_ERROR) 
		{
			break;
		}
		else if (result > 0) 
		{
			HandleEvents(readFds, writeFds);
		}
	}

	Stop();
	printf("IoContext stop!\n");
}

void IoContext::Attach(Socket* socket)
{
	m_sockets.insert(socket);
}

void IoContext::Detach(Socket* socket)
{
	m_sockets.erase(socket);

	UnWatchRead(socket);
	UnWatchWrite(socket);
}

void IoContext::WatchRead(Socket* socket)
{
	//设置读状态
	//socket->GetState() |= 1;

	//注册
	FD_SET(socket->GetFd(), &m_readFds);
}

void IoContext::UnWatchRead(Socket* socket)
{
	//socket->GetState() &= ~1;

	FD_CLR(socket->GetFd(), &m_readFds);
}

void IoContext::WatchWrite(Socket* socket)
{
	//socket->GetState() |= 2;

	FD_SET(socket->GetFd(), &m_writeFds);
}

void IoContext::UnWatchWrite(Socket* socket)
{
	//socket->GetState() &= ~2;

	FD_CLR(socket->GetFd(), &m_writeFds);
}

void IoContext::HandleEvents(const fd_set& readFds, const fd_set& writeFds)
{
	std::vector<Socket*> triggeredRead, triggeredWrite;

	for (Socket* s : m_sockets) 
	{
		if (FD_ISSET(s->GetFd(), &readFds)) 
		{
			triggeredRead.push_back(s);
		}
		if (FD_ISSET(s->GetFd(), &writeFds))
		{
			triggeredWrite.push_back(s);
			UnWatchWrite(s);
		}
	}

	//IO准备就绪,恢复挂起的协程
	for (Socket* s : triggeredRead) 
	{
		s->ResumeRecv();
	}

	for (Socket* s : triggeredWrite)
	{
		s->ResumeSend();
	}
}

