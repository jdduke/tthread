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

#ifndef _TINYTHREAD_QUEUE_H_
#define _TINYTHREAD_QUEUE_H_

#include "tinythread.h"

#include <queue>

namespace tthread {

template <typename T>
class concurrent_queue {
public:
	concurrent_queue()  : mDestroy(false) { }
	~concurrent_queue() { }

	void push(const T& data) {
		lock guard(mLock);
		if (mDestroy)
			return;

		mQ.push(data);
		// Unlock?
		mCondition.notify_one();
	}
	void push(T&& data) {
		lock guard(mLock);
		if (mDestroy)
			return;

		mQ.emplace(std::move(data));
		// Unlock?
		mCondition.notify_one();
	}

	bool empty() const {
		lock guard(mLock);
		return mQ.empty();
	}

	bool try_pop(T& value) {
		lock guard(mLock);

		if(mDestroy || mQ.empty()) {
			return false;
		}

		value = mQ.front();
		mQ.pop();
		return true;
	}

	bool wait_and_pop(T& value) {
		lock guard(mLock);

		while(!mDestroy && mQ.empty()) {
			mCondition.wait(mLock);
		}

		if (mDestroy)
			return false;

		value = std::move(mQ.front());
		mQ.pop();
		return true;
	}

	void destroy() {
		lock guard(mLock);
		mDestroy = true;
		while (!mQ.empty()) 
			mQ.pop();
		mCondition.notify_all();
	}

private:
	_TTHREAD_DISABLE_ASSIGNMENT(concurrent_queue);

	std::queue<T>        mQ;
	mutable future_mutex mLock;
	condition_variable   mCondition;
	bool                 mDestroy;
};

}

#endif // _TINYTHREAD_QUEUE_H_