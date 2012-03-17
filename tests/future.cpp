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

#define USE_TTHREAD 1
#if USE_TTHREAD
#include <tinythread.h>
#include <tinythread_experimental.h>
#else
#include <future>
#endif

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

#if USE_TTHREAD
using namespace tthread;
#define SYNC_FLAGS
#else
#define SYNC_FLAGS std::launch::async,
#endif

int ackermann(int m, int n) {
  if(m==0) return n+1;
  if(n==0) return ackermann(m-1,1);
  return ackermann(m-1, ackermann(m, n-1));
}

int main() {

  cout << "Main thread id: " << this_thread::get_id() << endl;
  vector<future<void>> futures;
  for (int i = 0; i < 8; ++i) {
    futures.emplace_back(async(SYNC_FLAGS [] {
      this_thread::sleep_for(chrono::seconds(1));
      cout << this_thread::get_id() << " ";
    }));
  }
  for_each(futures.begin(), futures.end(), [](future<void> & f) {
    f.wait();
  });
  cout << endl;

  ///////////////////////////////////////////////////////////////////////////

  packaged_task<int()> task(bind(&ackermann,3,11));
  auto f = task.get_future();
  task();
  cout << "Ackerman(3,11) = " << f.get() << endl;

  ///////////////////////////////////////////////////////////////////////////

  vector<future<int>> futures2;
  for (int i = 0; i < 8; ++i) {
    futures2.emplace_back(async(SYNC_FLAGS &ackermann,3,11));
  }
  for_each(futures2.begin(), futures2.end(), [=](future<int> & f) {
    std::cout << "Ackerman(3,11) = " << f.get() << endl;
  });
  cout << endl;

}
