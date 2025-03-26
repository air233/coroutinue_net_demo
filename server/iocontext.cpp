#include "iocontext.h"
#include <Winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "socket.h"
#include <csignal>

// ����̨�¼�������
BOOL WINAPI IoContext::ConsoleCtrlHandler(DWORD ctrlType)
{
	switch (ctrlType) {
	case CTRL_C_EVENT: // Ctrl+C �¼�
		printf("Ctrl+C pressed. Exiting gracefully...\n");
		// ���������ִ�ж�����������
		return TRUE; // ���� TRUE ��ʾ�Ѵ�����¼�
	case CTRL_CLOSE_EVENT: // ����̨�ر��¼�
		printf("Console close event received. Exiting...\n");
		return TRUE;
	default:
		return FALSE; // ���� FALSE ��ʾδ������¼�
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

	//TODO:ע��ctrl+c�¼� �˳�����
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
		timeout.tv_usec = 10000; // 10ms��ʱ������CPUռ��

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
	//���ö�״̬
	//socket->GetState() |= 1;

	//ע��
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

	//IO׼������,�ָ������Э��
	for (Socket* s : triggeredRead) 
	{
		s->ResumeRecv();
	}

	for (Socket* s : triggeredWrite)
	{
		s->ResumeSend();
	}
}

