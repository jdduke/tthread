/*
Copyright (c) 2012 Jared Duke

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef _TINYTHREAD_ASYNC_H_
#define _TINYTHREAD_ASYNC_H_

/// @file

#include "tinythread_future.h"
#include "tinythread_t.h"

namespace tthread {

///////////////////////////////////////////////////////////////////////////
// launch
namespace launch {
enum policy {
    async    = 0x01,
    deferred = 0x02,
    sync     = deferred,
    any      = async | deferred
};
};

///////////////////////////////////////////////////////////////////////////

template< typename Task >
auto async_impl(launch::policy policy, Task&& task) -> decltype(task.get_future()) {
	auto future = task.get_future();
	if ((policy & launch::async) != 0) {
		threadt thread(std::move(task));
		thread.detach();
	}
	return future;
}

template< typename F >
auto async(launch::policy policy, const F& f) -> future<decltype(f())> {
	typedef decltype(f())                result_type;
	typedef packaged_task<result_type()> task_type;
	return async_impl(policy, task_type(f));
}

template< typename F >
auto async(launch::policy policy, F&& f) -> future<decltype(f())> {
	typedef decltype(f())                result_type;
	typedef packaged_task<result_type()> task_type;
	return async_impl(policy, task_type(std::move(f)));
}

template< typename F >
auto async(const F& f) -> future<decltype(f())> {
	return async(launch::any, f);
}

template< typename F >
auto async(F&& f) -> future<decltype(f())> {
	return async(launch::any, std::move(f));
}

#if defined(_TTHREAD_VARIADIC_)

template< typename F, typename... Args >
auto async(F f, Args&& ... args) -> future<decltype(f(args...))> {
	return async(std::bind(f, std::move(args)...));
}

#else

template< typename F, typename T >
auto async(F f, T&& t) -> future<decltype(f(t))> {
	return async(std::bind(f, std::move(t)));
}

template< typename F, typename T, typename U >
auto async(F f, T&& t, U&& u) -> future<decltype(f(t, u))> {
	return async(std::bind(f, std::move(t), std::move(u)));
}

template< typename F, typename T, typename U, typename V >
auto async(F f, T&& t, U&& u, V&& v) -> future<decltype(f(t, u, v))> {
	return async(std::bind(f, std::move(t), std::move(u), std::move(v)));
}

template< typename F, typename T, typename U, typename V, typename W >
auto async(F f, T&& t, U&& u, V&& v, W&& w) -> future<decltype(f(t, u, v, w))> {
	return async(std::bind(f, std::move(t), std::move(u), std::move(v)));
}

#endif

} // namespace tthread

#endif // _TINYTHREAD_ASYNC_H_
