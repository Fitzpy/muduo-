// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/CountDownLatch.h>

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{
}

void CountDownLatch::wait()//阻塞等待
{
  MutexLockGuard lock(mutex_);//上锁，当这个代码块运行结束，析构函数自动解锁
  while (count_ > 0) {
    condition_.wait();//阻塞等待条件满足
  }
}

void CountDownLatch::countDown()
{
  MutexLockGuard lock(mutex_);//上锁
  --count_;
  if (count_ == 0) {
    condition_.notifyAll();//广播所有线程
  }
}

int CountDownLatch::getCount() const//返回计数值
{
  MutexLockGuard lock(mutex_);
  return count_;
}

