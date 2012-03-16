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

#ifndef _TINYTHREAD_EXPERIMENTAL_H_
#define _TINYTHREAD_EXPERIMENTAL_H_

/// @file

#include "tinythread.h"
#include "tinythread_t.h"

#if !defined(_TTHREAD_FUNCTIONAL_)
# error std::function must exist
#else
#define _TTHREAD_EXPERIMENTAL_
#endif

#include <memory>

// Macro for disabling assignments of objects.
#ifdef _TTHREAD_CPP0X_PARTIAL_
#define _TTHREAD_DISABLE_ASSIGNMENT(name) \
  name(const name&) = delete; \
  name& operator=(const name&) = delete;
#else
#define _TTHREAD_DISABLE_ASSIGNMENT(name) \
  name(const name&); \
  name& operator=(const name&);
#endif

namespace tthread {

template< class >
class packaged_task;

template < class >
class future;

///////////////////////////////////////////////////////////////////////////

template< class R >
struct async_result {

  template< class T >
  struct result { typedef T type; };
  template< >
  struct result<void> { typedef bool type; };

  std::unique_ptr<typename result<R>::type > mResult;
  mutable mutex      mResultLock;
  condition_variable mResultCondition;
  bool               mException;

  bool ready() const { 
    lock_guard<mutex> guard(mResultLock);
    return mResult || mException;
  }

  template<class> friend class packaged_task;

protected:
  async_result() : mException(false) { }

  _TTHREAD_DISABLE_ASSIGNMENT(async_result);
};

///////////////////////////////////////////////////////////////////////////

template< class R >
class packaged_task<R()>
{
public:
  typedef R result_type;

  ///////////////////////////////////////////////////////////////////////////
  // construction and destruction

  packaged_task() { }
  ~packaged_task() { }

  explicit packaged_task(R(*f)())    : mFunc( f ) { }
  template <class F>
  explicit packaged_task(const F& f) : mFunc( f ) { }
  template <class F>
  explicit packaged_task(F&& f)      : mFunc( std::move( f ) ) { }

  ///////////////////////////////////////////////////////////////////////////
  // move support

  packaged_task(packaged_task&& other)
  {
    *this = std::move(other);
  }

  packaged_task& operator=(packaged_task&& other) 
  {
    swap( std::move(other) );
    return *this;
  }

  void swap(packaged_task&& other)
  {
    lock_guard<mutex> guard(mLock);
    std::swap(mFunc,   other.mFunc);
    std::swap(mResult, other.mResult);
  }

  ///////////////////////////////////////////////////////////////////////////
  // result retrieval

  operator bool() const 
  { 
    lock_guard<mutex> guard(mLock);
    return mFunc; 
  }

  future<R> get_future()
  {
    lock_guard<mutex> guard(mLock);
    if (!mResult)
      mResult.reset( new async_result<R>() );
    return future<R>(mResult);
  }

  ///////////////////////////////////////////////////////////////////////////
  // execution

  void operator()(void*) { (*this)(); }
  void operator()();

  void reset();

private:
  _TTHREAD_DISABLE_ASSIGNMENT(packaged_task);

  mutable mutex mLock;
  std::function<R()> mFunc;
  std::shared_ptr< async_result<R> > mResult;
};

///////////////////////////////////////////////////////////////////////////

template< class T, class F, class R >
struct package_helper {
  static void get(T& receiver, F& func) {
    receiver.reset( new R( func() ) );
  }
};

template< class T, class F >
struct package_helper<T,F,void> {
  static void get(T& receiver, F& func) {
    func();
    receiver.reset( new bool(true) );
  }
};

template< class R >
void tthread::packaged_task<R()>::operator()()
{
  if (!(*this))
    return;

  std::shared_ptr< async_result<R> > result;
  {
    lock_guard<mutex> guard(mLock);
    if (!mResult)
      mResult.reset( new async_result<R>() );
    result = mResult;
  }

  lock_guard<mutex> guardResult(result->mResultLock);
  if(!result->mResult)
    package_helper<decltype(result->mResult), decltype(mFunc), R>::get(result->mResult, mFunc);
    //result->mResult.reset( new R( mFunc() ) );

  result->mResultCondition.notify_all();
}

template< class R >
void tthread::packaged_task<R()>::reset()
{
  lock_guard<mutex> guard(mLock);

#if 0
  if (mResult && !mResult->ready())
  {
    lock_guard<mutex> guardResult(mResult->mResultLock);
    mResult->mException = true;
  }
#endif

  mResult.reset( );
}

///////////////////////////////////////////////////////////////////////////

/// Future class.
template< class R >
class future {
public:
  future()  { }
  ~future() { }
  future(future<R>&& f) : mResult( f.mResult ) { }
  future& operator=(future&& other) { std::swap(mResult, other.mResult); }
  
  bool valid() const     { return mResult; }
  bool is_ready() const  { return valid() && result->ready(); }
  bool has_value() const { is_ready(); }

  R    get();
  void wait();

  template<class> friend class packaged_task;

protected:
  future(std::shared_ptr< async_result<R> >& result) : mResult(result) { }

  _TTHREAD_DISABLE_ASSIGNMENT(future)

  std::shared_ptr< async_result<R> > mResult;
};

///////////////////////////////////////////////////////////////////////////

template< class R >
struct get_helper { static R get(R* r) { return *r; } };
template< >
struct get_helper<void> { static void get(...) { } };

template< class R >
typename R tthread::future<R>::get()
{
  wait();

  if (!valid())
    throw std::exception("invalid future");

  lock_guard<mutex> guard(mResult->mResultLock);
  std::shared_ptr< async_result<R> > result = mResult;
  if(result->mException || !result->mResult)
    throw std::exception("invalid future");
  return get_helper<R>::get(result->mResult.get());
}

template< class R >
void tthread::future<R>::wait()
{
  std::shared_ptr< async_result<R> > result = mResult;

  if (result && !result->ready())
  {
    lock_guard<mutex> guard(result->mResultLock);
    while (!result->mResult && !result->mException)
    {
      result->mResultCondition.wait(result->mResultLock);
    }
  }
}

///////////////////////////////////////////////////////////////////////////

template< class F >
auto async(F f) -> future<decltype(f())>
{
  typedef decltype(f())                result_type;
  typedef packaged_task<result_type()> task_type;
  typedef future<result_type>          future_type;

  task_type task(std::move(f));
  auto future = task.get_future();
  threadt thread(std::move(task));
  thread.detach();
  return future;
}

template< class F, class T >
auto async(F f, T t) -> future<decltype(f(t))>
{
  return async(std::bind(f, t));
}

template< class F, class T, class U >
auto async(F f, T t, U u) -> future<decltype(f(t,u))>
{
  return async(std::bind(f, t, u));
}

template< class F, class T, class U, class V >
auto async(F f, T t, U u, V v) -> future<decltype(f(t,u,v))>
{
  return async(std::bind(f, t, u, v));
}

#if 0
template < class Function >
std::future< typename std::result_of< Function() >::type >
async( Function&& f ) 
{
  return future();
}

template < class Function, class T >
std::future< typename std::result_of< Function(T) >::type >
async( Function&& f, T&& t );

template < class Function, class T, class U >
std::future< typename std::result_of< Function(T,U) >::type >
async( Function&& f, T&& t, U&& u );

#endif

}

#undef _TTHREAD_DISABLE_ASSIGNMENT

#endif // _TINYTHREAD_EXPERIMENTAL_H_
