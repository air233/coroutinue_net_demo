#pragma once

#include <coroutine>
#include <iostream>


using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

template<typename T> struct task;

namespace detail {
	template<typename T>
	struct promise_type_base
	{
		coroutine_handle<> m_coro = std::noop_coroutine();

		//协程框架接口
		task<T> get_return_object();

		suspend_always initial_suspend() { return {}; }
		
		struct finally_awaiter
		{
			bool await_ready() noexcept { return false; }//不销毁
			void await_resume() noexcept {}

			template<typename promise_type>
			coroutine_handle<> await_suspend(coroutine_handle<promise_type> coro) noexcept
			{
				return coro.promise().m_coro;
			}
		};

		auto final_suspend() noexcept
		{
			return finally_awaiter{};
		}

		void unhandled_exception()
		{
			exit(-1);
		}

	};

	//=================================================================

	//定义promies
	template<typename T>
	struct promise_type final : promise_type_base<T>
	{
		T m_result;
		
		void return_value(T val) { m_result = val; }

		T awaiter_resule() { return m_result; }

		task<T>  get_return_object();
	};

	//特例化
	template<>
	struct promise_type<void>  final: promise_type_base<void>
	{
		void return_void() {}

		void awaiter_resume() {}

		task<void> get_return_object();
	};
};

template<typename T = void>
struct task
{
	using promise_type = detail::promise_type<T>;

	task() :m_handle(nullptr) {}
	task(coroutine_handle<promise_type> handle) :m_handle(handle) {}
	//~task()
	//{
	//	if (m_handle && m_handle.done())
	//	{
	//		m_handle.destroy();
	//	}
	//}

	bool await_ready() { return false; }
	T await_resume() { return m_handle.promise().m_result; }

	coroutine_handle<promise_type> await_suspend(coroutine_handle<> waiter)
	{
		//记录挂起协程,返回当前协程继续执行
		m_handle.promise().m_coro = waiter;

		return m_handle;
	};

	//唤醒协程
	void resume()
	{
		m_handle.resume();
	}

	coroutine_handle<promise_type> m_handle;
};

template<typename T>
inline task<T> detail::promise_type<T>::get_return_object()
{
	return task<T>{ coroutine_handle<detail::promise_type<T>>::from_promise(*this)};
}

inline task<void> detail::promise_type<void>::get_return_object()
{
	return task<void>{ coroutine_handle<detail::promise_type<void>>::from_promise(*this)};
}
