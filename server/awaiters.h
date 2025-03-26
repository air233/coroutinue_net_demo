#pragma once
#include <coroutine>
#include "socket.h"


template<typename SysIOCall, typename ReturnValue>
class Awaiter
{
public:
	Awaiter() : m_suspend(false) {}
	
	//���غ����̹���
	bool await_ready() const noexcept { return false; }

	//��������
	bool await_suspend(std::coroutine_handle<> handle) noexcept
	{
		static_assert(std::is_base_of_v<Awaiter, SysIOCall>(handle));

		m_handle = handle;

		//ִ�в���,�����Ƿ�����
		m_suspend = static_cast<SysIOCall*>(this)->SysIOCall();

		if (m_suspend)
		{
			//���õ�ǰЭ��
			static_cast<SysIOCall*>(this)->SetCoroHandle();
		}

		return m_suspend;
	}
	
	//�ָ��󷵻�ֵ
	ReturnValue await_resume() noexcept
	{
		if (m_suspend)
		{
			//��������ʱ, ��ʱIO�Ѿ�׼������,�ٵ���һ��IO
			static_cast<SysIOCall*>(this)->SysIOCall();
		}

		//����ֵ
		return m_value;
	}

	//��¼����ֵ
	void SetReturnValue(ReturnValue& val) { m_value = val; }

protected:
	bool m_suspend;
	std::coroutine_handle<> m_handle;
	ReturnValue m_value;
};



class Socket;

class AcceptAwaiter : public Awaiter<AcceptAwaiter, SOCKET>
{
public:
	AcceptAwaiter(Listener* listen);
	~AcceptAwaiter();

	bool SysIOCall();
	void SetCoroHandle() { m_listen->GetScoket().SetRecvCo(m_handle); }
private:
	Listener* m_listen;
};

class RecvAwaiter : public Awaiter<RecvAwaiter, int>
{
public:
	RecvAwaiter(Socket* socket, void* buff, int len);
	~RecvAwaiter();

	bool SysIOCall();
	void SetCoroHandle() { m_socket->SetRecvCo(m_handle); }
private:
	Socket* m_socket;
	void* m_buffer;
	int m_len;
};

class SendAwaiter : public Awaiter<SendAwaiter, int>
{
public:
	SendAwaiter(Socket* socket, void* buff, int len);
	~SendAwaiter();

	bool SysIOCall();
	void SetCoroHandle() { m_socket->SetSendCo(m_handle); }

private:
	Socket* m_socket;
	void* m_buffer;
	int m_len;
};
