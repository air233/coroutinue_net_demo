#include <iostream>
#include "task.h"
#include "iocontext.h"
#include "socket.h"
#include "awaiters.h"

#pragma comment(lib, "ws2_32.lib")

task<bool> echo_logic(Socket& socket)
{
	char buffer[1024] = { 0 };
	char* pbuffer = buffer;
	
	//等待接收数据
	int len = co_await socket.Recv(&buffer, sizeof(buffer));

	if (len == 0)
	{
		std::cout << "disconnct fd: " << socket.GetFd() << "\n";
		co_return false;
	}

	std::cout << "recv data: " << buffer << "\n";
	buffer[strlen(buffer)] = '\n';

	int send_len = 0;
	while (send_len < len)
	{
		pbuffer += send_len;
		int send_l = co_await socket.Send(pbuffer, len - send_len);
		if (send_l <= 0) 
		{
			co_return false;
		}
		send_len += send_l;
	}

	co_return true;
}


task<> echo_socket(std::shared_ptr<Socket> socket)
{
	for (;;)
	{
		bool b = co_await echo_logic(*socket);

		if (!b) break;
	}
}

task<> accpet(Listener& listnner)
{
	for (;;)
	{
		auto socket = co_await listnner.Accpet();
		auto t = echo_socket(socket);
		t.resume();
	}
}


int main()
{
	IoContext ioContex;
	ioContex.Start();

	Listener listener("0.0.0.0", "8888", ioContex);
	auto t = accpet(listener);
	t.resume();

	ioContex.Run();
	return 0;
}