#pragma once
#include "task.h"
#include "iocontext.h"
#include <Winsock2.h>
#include <windows.h>
#include <memory>
#include <coroutine>

class RecvAwaiter;
class SendAwaiter;

class Socket 
{
public:
	Socket(IoContext& io_context) : m_iocontex(io_context) {}
	Socket(SOCKET fd, IoContext& io_context);
	Socket(Socket&& socket);
	~Socket();
	Socket(const Socket& socket) = delete;

	// 赋值运算符
	Socket& operator=(Socket&& other) noexcept;

	// 禁用拷贝赋值运算符
	Socket& operator=(const Socket& other) = delete;

	void RegisterIoContext();
	void RegisterRead();
	void UnRegisterRead();
	void RegisterWrite();
	void UnRegisterWrite();

	RecvAwaiter Recv(void* buff, int len);
	SendAwaiter Send(void* buff, int len);
	void ResumeRecv();
	void ResumeSend();
	void SetNonBlocking();

	void SetRecvCo(std::coroutine_handle<> h) { m_coro_recv = h; }
	void SetSendCo(std::coroutine_handle<> h) { m_coro_send = h; }
	void SetIoContext(IoContext& io_context) {	m_iocontex = io_context; }
	IoContext& GetIoContext() { return m_iocontex; }
	const SOCKET& GetFd() const { return m_socket_fd; }
	int32_t& GetState() { return m_io_state; }

private:
	IoContext& m_iocontex;
	int32_t m_io_state = 0;
	SOCKET m_socket_fd = -1;

	std::coroutine_handle<> m_coro_recv;
	std::coroutine_handle<> m_coro_send;
};


class Listener
{
public:
	Listener(const std::string& addr, const std::string& port,IoContext& io_context,bool reuseport = false, int listqueue = 5);
	Listener(const std::string& port, IoContext& io_context, bool reuseport = false, int listqueue = 5);
	~Listener();

	Socket& GetScoket() { return m_socket; }

	task<std::shared_ptr<Socket>> Accpet();

protected:
	void Initialize(IoContext& io_context, const std::string& addr, const std::string& port, bool reuseport, int listqueue);

private:
	Socket m_socket;
};

//TODO:先做简单的

//class Connect : public Socket 
//{
//
//};