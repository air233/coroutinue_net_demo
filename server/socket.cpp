#include "socket.h"
#include "iocontext.h"
#include <coroutine>
#include "awaiters.h"
#include <winsock2.h>
#include <ws2tcpip.h>

Socket::Socket(SOCKET fd, IoContext& io_context) :m_iocontex(io_context), m_socket_fd(fd)
{
	
}

Socket::Socket(Socket&& socket): m_iocontex(socket.m_iocontex),m_io_state(socket.m_io_state),m_socket_fd(socket.m_socket_fd),
m_coro_recv(socket.m_coro_recv),m_coro_send(socket.m_coro_send)
{
	socket.m_io_state = 0;
	socket.m_socket_fd = INVALID_SOCKET;
	socket.m_coro_recv = std::noop_coroutine();
	socket.m_coro_send = std::noop_coroutine();
}

Socket& Socket::operator=(Socket&& socket) noexcept
{
	if (this != &socket)
	{
		m_iocontex = socket.m_iocontex;
		m_io_state = socket.m_io_state;
		m_socket_fd = socket.m_socket_fd;
		m_coro_recv = socket.m_coro_recv;
		m_coro_send = socket.m_coro_send;

		socket.m_io_state = 0;
		socket.m_socket_fd = INVALID_SOCKET;
		socket.m_coro_recv = std::noop_coroutine();
		socket.m_coro_send = std::noop_coroutine();
	}
	
	return *this;
}

void Socket::RegisterIoContext()
{
	std::cout << "RegisterIoContext fd:" << GetFd() << "\n";

	m_iocontex.Attach(this);
	m_iocontex.WatchRead(this);
}

void Socket::RegisterRead()
{
	m_iocontex.WatchRead(this);
}

void Socket::UnRegisterRead()
{
	m_iocontex.UnWatchRead(this);
}

void Socket::RegisterWrite()
{
	m_iocontex.WatchWrite(this);
}

void Socket::UnRegisterWrite()
{
	m_iocontex.UnWatchWrite(this);
}

Socket::~Socket()
{
	std::cout << "Socket::~Socket:" << m_socket_fd << "\n";
	m_iocontex.Detach(this);
}

RecvAwaiter Socket::Recv(void* buff, int len)
{
	return RecvAwaiter{ this, buff, len };
}

SendAwaiter Socket::Send(void* buff, int len)
{
	return SendAwaiter{ this, buff, len };
}

void Socket::ResumeRecv()
{
	if (!m_coro_recv)
	{
		std::cout << "no handle for recv\n";
		return;
	}

	m_coro_recv.resume();
}

void Socket::ResumeSend()
{
	if (!m_coro_send)
	{
		std::cout << "no handle for send\n";
		return;
	}

	m_coro_send.resume();
}

void Socket::SetNonBlocking()
{
	u_long mode = 1; // 1 表示非阻塞模式，0 表示阻塞模式
	if (ioctlsocket(m_socket_fd, FIONBIO, &mode) != 0) 
	{
		printf("Failed to set socket to non-blocking mode!\n");
	}
}

Listener::Listener(const std::string& addr, const std::string& port, IoContext& io_context, bool reuseport /*= false*/, int listqueue /*= 5*/)
	: m_socket(io_context)
{
	Initialize(io_context, addr, port, reuseport, listqueue);

	//注册到IoContext
	m_socket.RegisterIoContext();
}

Listener::Listener(const std::string& port, IoContext& io_context, bool reuseport /*= false*/, int listqueue /*= 5*/)
	: m_socket(io_context)
{
	Initialize(io_context, "", port, reuseport, listqueue);

	//注册到IoContext
	m_socket.RegisterIoContext();
}

Listener::~Listener()
{

}

task<std::shared_ptr<Socket>> Listener::Accpet()
{
	//等待返回一个已连接的FD
	SOCKET fd = co_await AcceptAwaiter{ this };

	if (fd == INVALID_SOCKET)
	{
		throw std::runtime_error("accpet error");
	}

	std::shared_ptr<Socket> socketPtr(new Socket(fd, m_socket.GetIoContext()));
	
	//设置非阻塞
	socketPtr->SetNonBlocking();

	//注册到IoContext
	socketPtr->RegisterIoContext();

	//返回一个新Socket对象
	co_return socketPtr;
}

void Listener::Initialize(IoContext& io_context, const std::string& addr, const std::string& port, bool reuseport, int listqueue)
{
	struct addrinfo hints, * res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;     // 支持 IPv4 和 IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP 套接字
	hints.ai_flags = AI_PASSIVE;     // 用于绑定

	if (getaddrinfo(addr.empty() ? nullptr : addr.c_str(), port.c_str(), &hints, &res) != 0) 
	{
		throw std::runtime_error("Failed to resolve address");
	}

	// 创建套接字
	SOCKET listen_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listen_socket == INVALID_SOCKET) 
	{
		freeaddrinfo(res);
		throw std::runtime_error("Failed to create socket");
	}

	// 设置 SO_REUSEADDR 和 SO_REUSEPORT（如果支持）
	int optval = 1;
	if (reuseport) 
	{
#ifdef SO_REUSEPORT
		if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&optval), sizeof(optval)) == -1) {
			std::cerr << "Warning: SO_REUSEPORT not supported" << std::endl;
		}
#else
		std::cerr << "Warning: SO_REUSEPORT not supported on this platform" << std::endl;
#endif
	}
	if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval)) == -1) {
		closesocket(listen_socket);
		freeaddrinfo(res);
		throw std::runtime_error("Failed to set SO_REUSEADDR");
	}

	Socket tempSocket(listen_socket, io_context);
	tempSocket.SetNonBlocking();
	m_socket = std::move(tempSocket);

	// 绑定套接字
	if (bind(listen_socket, res->ai_addr, res->ai_addrlen) == -1) 
	{
		closesocket(listen_socket);
		freeaddrinfo(res);
		throw std::runtime_error("Failed to bind socket");
	}

	// 开始监听
	if (listen(listen_socket, listqueue) == -1) 
	{
		closesocket(listen_socket);
		freeaddrinfo(res);
		throw std::runtime_error("Failed to listen on socket");
	}
	// 释放地址信息
	freeaddrinfo(res);

	std::cout << "Listening on " << (addr.empty() ? "0.0.0.0" : addr) << ":" << port << std::endl;
}
