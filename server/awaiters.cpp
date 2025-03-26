#include "awaiters.h"
#include "iostream"
#include "socket.h"
#include "iocontext.h"

AcceptAwaiter::AcceptAwaiter(Listener* listen) : Awaiter{},m_listen(listen)
{
	//监听socket
	m_listen->GetScoket().RegisterRead();
	//std::cout << " socket accept opertion\n";
}

AcceptAwaiter::~AcceptAwaiter()
{
	m_listen->GetScoket().UnRegisterRead();
	//std::cout << " ~socket accept opertion\n";
}

bool AcceptAwaiter::SysIOCall()
{
	struct sockaddr_storage addr;
	int addr_size = sizeof addr;
	std::cout << "accpet: " << m_listen->GetScoket().GetFd() << std::endl;

	SOCKET socket = ::accept(m_listen->GetScoket().GetFd(), (sockaddr*)&addr, &addr_size);

	if (socket == INVALID_SOCKET)
	{
		//无连接时，直接挂起
		return true;
	}

	//当有连接时，设置返回值
	SetReturnValue(socket);

	return false;
}


RecvAwaiter::RecvAwaiter(Socket* socket, void* buff, int len) : Awaiter{}, m_socket(socket), m_buffer(buff), m_len(len)
{
	m_buffer = buff;
	m_len = len;

	m_socket->RegisterRead();
}

RecvAwaiter::~RecvAwaiter()
{
	m_socket->UnRegisterRead();
}

bool RecvAwaiter::SysIOCall()
{
	std::cout << "recv fd:" << m_socket->GetFd() << "\n";

	int recv_ = ::recv(m_socket->GetFd(), (char*)m_buffer, m_len, 0);

	if (recv_ == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
	{
		return true;
	}

	//std::cout << "RecvAwaiter::SysIOCall| recv:" << recv_ << "\n";

	SetReturnValue(recv_);
	return false;
}


SendAwaiter::SendAwaiter(Socket* socket, void* buff, int len) : Awaiter{}, m_socket(socket), m_buffer(buff), m_len(len)
{
	m_buffer = buff;
	m_len = len;
}

SendAwaiter::~SendAwaiter()
{
	//TODO:判断一下是否注册写事件
	m_socket->GetIoContext().UnWatchWrite(m_socket);
}

bool SendAwaiter::SysIOCall()
{
	std::cout << "send:" << m_socket->GetFd() << "\n";

	int send_ = ::send(m_socket->GetFd(), (char*)m_buffer, m_len, 0);
	if (send_ == SOCKET_ERROR)
	{
		//完全不能写入时
		return true;
	}

	if (send_ < m_len)
	{
		//发送不全的时候, 关注可写事件
		m_socket->GetIoContext().WatchWrite(m_socket);
	}

	//std::cout << "SendAwaiter::SysIOCall| send data size:" << send_ << "\n";
	SetReturnValue(send_);
	return false;
}

