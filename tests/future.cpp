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

#include <iostream>
#include <tinythread.h>
#include <tinythread_experimental.h>

#include <algorithm>
#include <vector>

using namespace std;
using namespace tthread;

float nine()
{
  return 9.f;
}

float sqr(int x)
{
#if defined(_DEBUG)
  cout << x << " -> " << x*x << endl;
#endif
  return (float)x*x;
}

template<typename Iterator,typename Func>
void parallel_for_each(Iterator first,Iterator last,Func f) {
  ptrdiff_t const range_length=last-first;
  if(!range_length)
    return;
  if(range_length==1) {
    f(*first);
    return;
  }

  Iterator const mid=first+(range_length/2);

  future<void> bgtask = async(&parallel_for_each<Iterator,Func>, first, mid, f);

  try {
    parallel_for_each(mid,last,f);
  }
  catch(...) {
    bgtask.wait();
    throw;
  }

  bgtask.get();
}

int main() {
  auto f = async(&nine);
  cout << " Result is: " << f.get() << endl;

  int i = 0;
  std::vector<int> test(15);
  std::generate(begin(test), end(test), [=]() mutable { return i++; });
  parallel_for_each(begin(test), end(test), &sqr);
}
