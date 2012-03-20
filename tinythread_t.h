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


#ifndef _TINYTHREAD_T_H_
#define _TINYTHREAD_T_H_

#include "tinythread.h"

#if defined(_TTHREAD_POSIX_)
#include <unistd.h>
#include <map>
#elif defined(_TTHREAD_WIN32_)
#include <process.h>
#endif

#include <memory>

/// @file
namespace tthread {

/// Thread class.
class threadt : public thread {
public:

	threadt() : thread() { }
	threadt(threadt&& other) : thread( std::move(other) ) { }
	threadt& operator=(threadt&& other) { 
		thread::swap(std::move(other)); 
		return *this;
	}

	template< typename thread_func_t >
	threadt(thread_func_t&& func) : thread() {
		init(std::move(func));
	}

#if defined(_TTHREAD_VARIADIC_)
	template< typename thread_func_t, typename Args... >
	threadt(thread_func_t&& func, Args&&... args) : thread() {
		init(std::bind(func, args...));
	}
#endif

protected:

	_TTHREAD_DISABLE_ASSIGNMENT(threadt);

	template< typename thread_func_t >
	void init(thread_func_t&& func);

	template< typename thread_func_t >
	struct _thread_start_info_t {
		thread_func_t    mFunction;   ///< Handle to the function to be executed.
		thread::data_ptr mThreadData; ///< Pointer to the thread data.
		_thread_start_info_t(thread_func_t&& func, thread::data_ptr threadData)
			: mFunction(std::move(func)), mThreadData(threadData) { }
	};

	template< class thread_func_t >
#if defined(_TTHREAD_WIN32_)
	static unsigned WINAPI wrapper_function(void* aArg);
#elif defined(_TTHREAD_POSIX_)
	static void* wrapper_function(void* aArg);
#endif

};

template< typename thread_func_t >
void threadt::init(thread_func_t&& func) {
	// Serialize access to this thread structure
	lock_guard<mutex> guard(mData->mMutex);

	// Fill out the thread startup information (passed to the thread wrapper,
	// which will eventually free it)
	typedef _thread_start_info_t<thread_func_t> thread_info;
	std::unique_ptr<thread_info> ti( new thread_info(std::move(func), mData) );

	thread_data& data = *mData;

	// The thread is now alive
	data.mNotAThread = false;

	// Create the thread
#if defined(_TTHREAD_WIN32_)
	data.mHandle = (HANDLE) _beginthreadex(0, 0, wrapper_function<thread_func_t>, (void*) ti.get(), 0, &data.mWin32ThreadID);
#elif defined(_TTHREAD_POSIX_)
	if (pthread_create(&data.mHandle, NULL, wrapper_function<thread_func_t>, (void*) ti.get()) != 0)
		data.mHandle = 0;
#endif

	// Did we fail to create the thread?
	if (!data.mHandle) {
		data.mNotAThread = true;
	} else {
		// Release ownership of the thread info object
		ti.release();
	}
}

template< class thread_func_t >
#if defined(_TTHREAD_WIN32_)
unsigned WINAPI threadt::wrapper_function(void* aArg)
#elif defined(_TTHREAD_POSIX_)
void* threadt::wrapper_function(void* aArg)
#endif
{
	typedef _thread_start_info_t<thread_func_t> thread_info;

	// Get thread startup information
	std::unique_ptr<thread_info> ti((thread_info*)aArg);

	try {
		ti->mFunction();
	} catch (...) {
		std::terminate();
	}

	lock_guard<mutex> guard(ti->mThreadData->mMutex);
	ti->mThreadData->mNotAThread = true;

	return 0;
}

}

#endif // _TINYTHREAD_T_H_
