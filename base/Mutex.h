// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*互斥锁类*/
#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
/*锁的基本操作（不可拷贝）*/
class MutexLock : boost::noncopyable
{
 public:
  MutexLock()
    : holder_(0)
  {
    int ret = pthread_mutex_init(&mutex_, NULL);//创建互斥锁
    assert(ret == 0); (void) ret;//如果assert函数值为0，则向stderr打印一条出错信息，然后通过调用 abort 来终止程序运行。
  }

  ~MutexLock()
  {
    assert(holder_ == 0);
    int ret = pthread_mutex_destroy(&mutex_);//销毁互斥锁
    assert(ret == 0); (void) ret;
  }

  bool isLockedByThisThread()
  {
    return holder_ == CurrentThread::tid();//判断是否被本线程上锁
  }

  void assertLocked()//判断当前线程
  {
    assert(isLockedByThisThread());
  }

  // internal usage

  void lock()//给当前线程上锁
  {
    pthread_mutex_lock(&mutex_);
    holder_ = CurrentThread::tid();
  }

  void unlock()//给当前线程解锁
  {
    holder_ = 0;
    pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t* getPthreadMutex() /* non-const *///返回private变量mutex_
  {
    return &mutex_;
  }

 private:

  pthread_mutex_t mutex_;//锁变量
  pid_t holder_;//记录持有锁的线程的ID
};

/*上锁和解锁封装在一个类的构造和析构函数中，这样保证上锁和解锁在一个同一个函数同一个代码块中，
 *只要在一个代码块中定义这个类变量,然后在这个代码块中就自动上锁，然后代码块结束，就自动释放锁*/
class MutexLockGuard : boost::noncopyable
{
 public:
  explicit MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
  {
    mutex_.lock();
  }

  ~MutexLockGuard()
  {
    mutex_.unlock();
  }

 private:

  MutexLock& mutex_;
};

}

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"
//不允许直接定义MutexLockGuard(mutex)这样的临时变量，如果直接定义了，就会报错

#endif  // MUDUO_BASE_MUTEX_H
