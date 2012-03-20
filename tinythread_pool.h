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

#ifndef _TINYTHREAD_POOL_H_
#define _TINYTHREAD_POOL_H_

/// @file

#include "tinythread_t.h"
#include "tinythread_future.h"
#include "tinythread_queue.h"

#include <vector>

namespace tthread {

///////////////////////////////////////////////////////////////////////////

template< typename T >
class concurrent_queue;

class thread_pool {
public:
	enum {
		DEFAULT_POOL_SIZE = 4,
	};

	// Construction and Destruction
	explicit thread_pool(unsigned thread_count = DEFAULT_POOL_SIZE);
	~thread_pool();

	// Submitting tasks for execution
	template<typename F>
	auto submit_task(const F& f) -> future<decltype(f())>;

	template<typename F>
	auto submit_task(F&& f) -> future<decltype(f())>;

	static thread_pool& instance() {
		static thread_pool pool(DEFAULT_POOL_SIZE);
		return pool;
	}

private:
	_TTHREAD_DISABLE_ASSIGNMENT(thread_pool);

	typedef packaged_task<void(void)>      Function;
	typedef concurrent_queue<Function>     FunctionQueue;
	typedef std::shared_ptr<FunctionQueue> FunctionQueuePtr;

	// TODO: Add lock for concurrent access from producers
	FunctionQueuePtr     mFunctionQueue;
	std::vector<threadt> mThreads;
};

///////////////////////////////////////////////////////////////////////////

template< typename Queue, typename Task > 
auto submit_task_impl(Queue& q, Task&& task) -> decltype(task.get_future()) {
	auto future = task.get_future();
	q.push(std::move(task));
	return future;
}

template< typename F >
auto thread_pool::submit_task(F&& f) -> future<decltype(f())> {
	typedef decltype(f())                result_type;
	typedef packaged_task<result_type()> task_type;
	return submit_task_impl(*mFunctionQueue, task_type(std::move(f)));
}

template< typename F >
auto tthread::thread_pool::submit_task(const F& f) -> future<decltype(f())> {
	typedef decltype(f())                result_type;
	typedef packaged_task<result_type()> task_type;
	return submit_task_impl(*mFunctionQueue, task_type(f));
}

} // namespace tthread

#endif // _TINYTHREAD_POOL_H_
