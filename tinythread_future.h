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

#ifndef _TINYTHREAD_FUTURE_H_
#define _TINYTHREAD_FUTURE_H_

/// @file

#include "tinythread.h"
#include "tinythread_t.h"
#include "fast_mutex.h"

#include <assert.h>
#include <memory>
#include <stdexcept>

#if defined(_MSC_VER)
namespace std {
template <typename T>
typename std::add_rvalue_reference<T>::type declval();
}
#else
#define _TTHREAD_VARIADIC_
#endif

namespace tthread {

///////////////////////////////////////////////////////////////////////////
// typedefs
typedef fast_mutex future_mutex;
typedef lock_guard<future_mutex> lock;

///////////////////////////////////////////////////////////////////////////
// forward declarations

#if defined(_TTHREAD_VARIADIC_)

template< typename... >
class packaged_task;

#else

template< typename >
class packaged_task;

#endif

template< typename >
class packaged_task_continuation;

template< typename >
class future;

///////////////////////////////////////////////////////////////////////////
// async_result

template< typename R >
struct result_helper {
	typedef R type;

	template< typename T, typename F >
	static void store(T& receiver, F& func) {
		receiver.set(func());
	}

#if defined(_TTHREAD_VARIADIC_)
	template< typename T, typename F, typename... Args >
	static void store(T& receiver, F& func, Args&&... args) {
		receiver.set(func(std::move(args)...));
	}
#else
	template< typename T, typename F, typename Arg >
	static void store(T& receiver, F& func, Arg&& arg) {
		receiver.set(func(std::move(arg)));
	}
#endif
	static R fetch(R* r) {
		return *r;
	}

	template< typename F, typename Arg >
	static void run(F& f, Arg& r) {
		f(r);
	}
};

template<>
struct result_helper<void> {
	typedef bool type;

	template< typename T, typename F >
	static void store(T& receiver, F& func) {
		func();
		receiver.set(true);
	}

#if defined(_TTHREAD_VARIADIC_)
	template< typename T, typename F, typename... Args >
	static void store(T& receiver, F& func, Args&& ... args) {
		func(std::move(args)...);
		receiver.set(true);
	}
#else
	template< typename T, typename F, typename Arg >
	static void store(T& receiver, F& func, Arg&& arg) {
		func(std::move(arg));
		receiver.set(true);
	}
#endif

	static void fetch(...) {

	}

	template< typename F >
	static void run(F& f, ...) {
		f();
	}
};

///////////////////////////////////////////////////////////////////////////

template <typename T, typename Mutex>
class locked_ptr {
public:
	locked_ptr(volatile T& obj, const volatile Mutex& lock)
		: mPtr(const_cast<T*>(&obj)),
		  mLock(*const_cast<Mutex*>(&lock)) {
		mLock.lock();
	}

	~locked_ptr() {
		mLock.unlock();
	}

	T* operator->() const {
		return mPtr;
	}

	T& operator*() const {
		return *mPtr;
	}

private:
	T* mPtr;
	Mutex& mLock;
};

///////////////////////////////////////////////////////////////////////////

template< typename R >
struct async_result {

	typedef typename result_helper<R>::type result_type;
	typedef async_result<R> this_type;

	~async_result() {
		//lock guard(mLock);
		//mCondition.notify_all();
	}

	bool ready() const volatile {
		return mReady;
	}

	void wait() const volatile {
		const this_type* self = const_cast<const this_type*>(this);
		lock guard(self->mLock);
		while (!mReady) {
			self->mCondition.wait(self->mLock);
		}
	}

	result_type operator()() const volatile {
		wait();

		return mResult;
	}

	void set(result_type&& r) volatile {
		locked_ptr<this_type, future_mutex> lockedSelf(*this, mLock);
		if (!mReady) {
			lockedSelf->mResult = std::move(r);
			lockedSelf->mReady  = true;
			lockedSelf->mCondition.notify_all();
			if (lockedSelf->mContinuation)
				result_helper<R>::run(*lockedSelf->mContinuation, lockedSelf->mResult);
		}
	}

	void setExecuting(bool executing) volatile {
		mExecuting = executing;
	}

	void setContinuation(packaged_task_continuation<R>* continuation) volatile {
		this_type* self = const_cast<this_type*>(this);
		lock guard(self->mLock);
		self->mContinuation.reset(continuation);
		if (self->mContinuation && mReady)
			(*self->mContinuation)(mResult);
	}

	template<typename, typename> friend class packaged_task_impl;
#if defined(_TTHREAD_VARIADIC_)
	template<typename...>        friend class packaged_task;
#endif

protected:
	async_result()
		: mReady(false), mExecuting(false), mException(false) { }

	_TTHREAD_DISABLE_ASSIGNMENT(async_result);

	volatile bool mReady;
	volatile bool mExecuting;
	volatile bool mException;

	volatile result_type mResult;

	mutable future_mutex       mLock;
	mutable condition_variable mCondition;

	std::unique_ptr< packaged_task_continuation<R> >   mContinuation;

};

///////////////////////////////////////////////////////////////////////////
// packaged_task

template< typename Arg >
class packaged_task_continuation {
public:
	virtual void operator()(Arg) = 0;
	virtual ~packaged_task_continuation() { }
};
template< >
class packaged_task_continuation< void > {
public:
	virtual void operator()(void) = 0;
	virtual ~packaged_task_continuation() { }
};

#if defined(_TTHREAD_VARIADIC_)

template < size_t N >
struct apply_func {
	template < typename F, typename... ArgsT, typename... Args >
	static void applyTuple(F f,
	                       const std::tuple<ArgsT...>& t,
	                       Args... args) {
		apply_func < N - 1 >::applyTuple(f, t, std::get < N - 1 > (t), args...);
	}
};

template <>
struct apply_func<0> {
	template < typename F, typename... ArgsT, typename... Args >
	static void applyTuple(F f,
	                       const std::tuple<ArgsT...>& /* t */,
	                       Args... args) {
		f(args...);
	}
};

template < typename F, typename... ArgsT >
void apply_tuple(F f,
                const std::tuple<ArgsT...>& t) {
	apply_func<sizeof...(ArgsT)>::applyTuple(f, t);
}

template < typename... Args >
struct first;
template < typename First, typename... Rest >
struct first<First,Rest...> { typedef First type; };
template < >
struct first<> { typedef void type; };

template< typename R, typename... Args >
class packaged_task<R(Args...)> : 
	public packaged_task_continuation< typename first<Args...>::type > {
public:
	typedef R result_type;

	///////////////////////////////////////////////////////////////////////////
	// construction and destruction

	packaged_task() { }
	~packaged_task() { }

	explicit packaged_task(R(*f)(Args...)) : mFunc(f) { }
	template < typename F >
	explicit packaged_task(F f)            : mFunc(std::move(f)) { }

	///////////////////////////////////////////////////////////////////////////
	// move support

	packaged_task(packaged_task&& other) {
		*this = std::move(other);
	}

	packaged_task& operator=(packaged_task&& other) {
		swap(std::move(other));
		return *this;
	}

	void swap(packaged_task&& other) {
		lock guard(mLock);
		std::swap(mFunc,   other.mFunc);
		std::swap(mResult, other.mResult);
	}

	///////////////////////////////////////////////////////////////////////////
	// result retrieval

	operator bool() const {
		lock guard(mLock);
		return !!mFunc;
	}

	future<R> get_future() {
		lock guard(mLock);
		if (!mResult)
			mResult.reset(new async_result<R>());
		return future<R>(mResult);
	}

	///////////////////////////////////////////////////////////////////////////
	// execution

	void operator()(Args... args)  {
		std::shared_ptr< async_result<R> > result;
		{
			lock guard(mLock);
			if (!!mFunc) {
				if (!mResult)
					mResult.reset(new async_result<R>());
				if (!mResult->mExecuting) {
					mResult->setExecuting(true);
					result = mResult;
				}
			}
		}

		if (result)
			result_helper<R>::store(*result, mFunc, args...);
	}

private:
	_TTHREAD_DISABLE_ASSIGNMENT(packaged_task);

	mutable future_mutex mLock;
	std::function<R(Args...)> mFunc;
	std::shared_ptr< async_result<R> > mResult;
};

#else

template< typename R, typename Arg >
class packaged_task_impl : public packaged_task_continuation<Arg> {
public:
	typedef R   result_type;
	typedef std::shared_ptr< async_result<result_type> > async_result_ptr;

	///////////////////////////////////////////////////////////////////////////
	// construction and destruction

	packaged_task_impl()  { }
	~packaged_task_impl() { }

	explicit packaged_task_impl(R(*f)(Arg)) : mFunc(f) { }
	template < typename F >
	explicit packaged_task_impl(F f)        : mFunc(std::move(f)) { }

	///////////////////////////////////////////////////////////////////////////
	// move support

	packaged_task_impl(packaged_task_impl && other) {
		*this = std::move(other);
	}

	packaged_task_impl& operator=(packaged_task_impl && other) {
		swap(std::move(other));
		return *this;
	}

	void swap(packaged_task_impl&& other) {
		lock guard(mLock);
		std::swap(mFunc,   other.mFunc);
		std::swap(mResult, other.mResult);
	}

	///////////////////////////////////////////////////////////////////////////
	// result retrieval

	operator bool() const {
		lock guard(mLock);
		return !!mFunc;
	}

	future<R> get_future() {
		lock guard(mLock);
		if (!mResult)
			mResult.reset(new async_result<R>());
		return future<R>(mResult);
	}
protected:
	_TTHREAD_DISABLE_ASSIGNMENT(packaged_task_impl);

	async_result_ptr& process(async_result_ptr& result) {
		if (*this) {
			lock guard(mLock);
			if (!mResult)
				mResult.reset(new async_result<R>());
			if (!mResult->mExecuting) {
				mResult->setExecuting(true);
				result = mResult;
			}
		}
		return result;
	}

	mutable future_mutex  mLock;
	std::function<R(Arg)> mFunc;
	async_result_ptr      mResult;
};

///////////////////////////////////////////////////////////////////////////

template< typename R >
class packaged_task<R()> : public packaged_task_impl<R, void> {
public:

	///////////////////////////////////////////////////////////////////////////
	// construction and destruction

	typedef packaged_task_impl<R, void> base;

	packaged_task() { }
	explicit packaged_task(R(*f)(void)) : base(f) { }
	template < typename F >
	explicit packaged_task(F f)         : base(std::move(f)) { }

	///////////////////////////////////////////////////////////////////////////
	// move support

	packaged_task(packaged_task&& other) {
		*this = std::move(other);
	}

	packaged_task& operator=(packaged_task&& other) {
		swap(std::move(other));
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	// execution

	void operator()();

private:
	_TTHREAD_DISABLE_ASSIGNMENT(packaged_task);
};

template< typename R >
void packaged_task<R()>::operator()() {
	std::shared_ptr< async_result<R> > result;
	if (process(result)) {
		result_helper<R>::store(*result, mFunc);
	}
}

///////////////////////////////////////////////////////////////////////////

template< typename R, typename Arg >
class packaged_task<R(Arg)> : public packaged_task_impl<R, Arg> {
public:

	///////////////////////////////////////////////////////////////////////////
	// construction and destruction

	typedef packaged_task_impl<R, Arg> base;

	packaged_task() { }
	explicit packaged_task(R(*f)(Arg)) : base(f) { }
	template < typename F >
	explicit packaged_task(F f)        : base(std::move(f)) { }

	///////////////////////////////////////////////////////////////////////////
	// move support

	packaged_task(packaged_task&& other) {
		*this = std::move(other);
	}

	packaged_task& operator=(packaged_task && other) {
		swap(std::move(other));
		return *this;
	}

	///////////////////////////////////////////////////////////////////////////
	// execution

	void operator()(Arg);

private:
	_TTHREAD_DISABLE_ASSIGNMENT(packaged_task);
};

template< typename R, typename Arg >
void packaged_task<R(Arg)>::operator()(Arg arg) {
	std::shared_ptr< async_result<R> > result;
	if (process(result)) {
		result_helper<R>::store(*result, mFunc, arg);
	}
}

#endif

///////////////////////////////////////////////////////////////////////////
/// Future class.

template< typename R >
class future {
public:
	typedef std::shared_ptr< async_result<R> > async_result_ptr;

	future(future<R>&& other) {
		std::swap(mResult, other.mResult);
	}
	future(async_result_ptr result) : mResult(result) { }
	~future() { }

	future& operator=(future&& other) {
		mResult.swap(other.mResult);
		return *this;
	}

	bool valid() const     {
		return !!mResult; 
	}

	bool is_ready() const  {
		return valid() && mResult->ready();
	}

	bool has_value() const {
		return is_ready();
	}

	R    get();
	void wait();

	template< typename F >
	auto then(F f) -> future<decltype(f(std::declval<R>()))>;

protected:

	future() { }
	_TTHREAD_DISABLE_ASSIGNMENT(future)

	async_result_ptr mResult;
};

///////////////////////////////////////////////////////////////////////////

template< typename R >
R future<R>::get() {
	return (*mResult)();
}

template< typename R >
void future<R>::wait() {
	mResult->wait();
}

template< typename R >
template< typename F >
auto future<R>::then(F f) -> future<decltype(f(std::declval<R>()))> {
	typedef decltype(f(std::declval<R>()))  result_type;
	typedef packaged_task< result_type(R) > task_type;

	std::unique_ptr<task_type> continuation(new task_type(std::move(f)));
	mResult->setContinuation(continuation.get());

	return continuation.release()->get_future();
}

} // namespace tthread

#endif // _TINYTHREAD_FUTURE_H_
