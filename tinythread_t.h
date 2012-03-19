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

    template< class thread_func_t >
    threadt(thread_func_t&& func) : thread() {
      // Serialize access to this thread structure
      lock_guard<mutex> guard(mDataMutex);

      // Fill out the thread startup information (passed to the thread wrapper,
      // which will eventually free it)
      auto ti = new _thread_start_info_t<thread_func_t>(std::move(func), nullptr, this);

      // The thread is now alive
      mNotAThread = false;

      // Create the thread
#if defined(_TTHREAD_WIN32_)
      mHandle = (HANDLE) _beginthreadex(0, 0, wrapper_function<thread_func_t>, (void *) ti, 0, &mWin32ThreadID);
#elif defined(_TTHREAD_POSIX_)
      if(pthread_create(&mHandle, NULL, wrapper_function<thread_func_t>, (void *) ti) != 0)
        mHandle = 0;
#endif

      // Did we fail to create the thread?
      if(!mHandle) {
        mNotAThread = true;
        delete ti;
      }
    }

protected:

  inline mutex& getLock() const {
    return mDataMutex;
  }
  inline void   setNotAThread(bool bNotAThread) {
    mNotAThread = bNotAThread;
  }

  template< typename thread_func_t >
  struct _thread_start_info_t {
    thread_func_t mFunction; ///< Handle to the function to be executed.
    void * mArg;             ///< Function argument for the thread function.
    threadt * mThread;       ///< Pointer to the thread object.
    _thread_start_info_t(thread_func_t&& func, void * arg, threadt * thread)
    : mFunction(std::move(func)), mArg(arg), mThread(thread) { }
  };

  template< class thread_func_t >
#if defined(_TTHREAD_WIN32_)
  static unsigned WINAPI wrapper_function(void * aArg);
#elif defined(_TTHREAD_POSIX_)
  static void * wrapper_function(void * aArg);
#endif

};

template< class thread_func_t >
#if defined(_TTHREAD_WIN32_)
unsigned WINAPI threadt::wrapper_function(void * aArg)
#elif defined(_TTHREAD_POSIX_)
void * threadt::wrapper_function(void * aArg)
#endif
{
  typedef _thread_start_info_t<thread_func_t> thread_info;

  // Get thread startup information
  std::unique_ptr<thread_info> ti( (thread_info*)aArg );

  try {
    ti->mFunction();
  } catch(...) {
    // Uncaught exceptions will terminate the application (default behavior
    // according to the C++0x draft)
    std::terminate();
  }

  // The thread is no longer executing
  //lock_guard<mutex> guard(ti->mThread->getLock());
  ti->mThread->setNotAThread(true);

  return 0;
}

}

#endif // _TINYTHREAD_T_H_
