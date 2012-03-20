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

#include "tinythread_pool.h"

using namespace tthread;

struct FunctionExecute {
	template< typename Task > 
	void operator()(Task& task) const {
		task();
	}
};

thread_pool::thread_pool( unsigned thread_count ) 
	: mFunctionQueue( new FunctionQueue() )
{
	FunctionQueuePtr q = mFunctionQueue;

	for (unsigned i = 0; i < thread_count; ++i) {
		mThreads.emplace_back( threadt( [=]() {
			Function f;
			while (q->wait_and_pop(f)) {
				f();
			}
		}) );
	}
}

tthread::thread_pool::~thread_pool() {
	/*
	Function f;
	while (mFunctionQueue->wait_and_pop(f)) {
		f();
	}
	*/
	while (!mFunctionQueue->empty())
		this_thread::sleep_for(chrono::milliseconds(100));
	for (size_t i = 0; i < mThreads.size(); ++i)
		mThreads[i].detach();
	mFunctionQueue->destroy();
}