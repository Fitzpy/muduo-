// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*将条件变量和锁两个类进一步封装，因为条件变量必须与互斥锁一起用才有价值
 *设置一个计数器，计数值为count_，如果count_变为0，就可以满足条件，然后解除条件等待
 */
#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{

class CountDownLatch : boost::noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  mutable MutexLock mutex_;//mutable关键词表示mutex_量是可以变的，即便在const函数中
  Condition condition_;//条件变量
  int count_;
};

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
