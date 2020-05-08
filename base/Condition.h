// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*条件变量操作封装类*/
#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

class Condition : boost::noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)//条件变量初始化
    : mutex_(mutex)
  {
    pthread_cond_init(&pcond_, NULL);
  }

  ~Condition()//条件变量销毁
  {
    pthread_cond_destroy(&pcond_);
  }

  void wait()//阻塞接受条件
  {
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());//pthread_cond_wait本意是可以选择不同的锁配合，但是作者认为
    //没有用到的必要，所以就在condition封装时，直接封装成一对一了
  }

  // returns true if time out, false otherwise.
  //时间等待，等待seconds秒，如果这段时间中没有触发条件，就会返回ETIMEOUT，并结束等待
  bool waitForSeconds(int seconds);

  void notify()//发出条件信号，每个线程都会被唤醒，但是只有一个线程的信号会解除阻塞
  {
    pthread_cond_signal(&pcond_);
  }

  void notifyAll()//向所有线程广播条件信号，每个线程都会被唤醒，并且每个线程都可以解除cond_wait阻塞
  {
    pthread_cond_broadcast(&pcond_);
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};

}
#endif  // MUDUO_BASE_CONDITION_H
