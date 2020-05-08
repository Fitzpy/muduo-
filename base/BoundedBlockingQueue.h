// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
#define MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <assert.h>

namespace muduo
{
/*有界阻塞队列，和BlockingQueue类似*/
template<typename T>
class BoundedBlockingQueue : boost::noncopyable
{
 public:
  explicit BoundedBlockingQueue(int maxSize)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      queue_(maxSize)
  {
  }
  //往队列中存放任务
  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    while (queue_.full())
    {
      notFull_.wait();//当队列满了，就阻塞等待，在take()函数中取完任务后，会发出条件变量信号
    }
    assert(!queue_.full());
    queue_.push_back(x);
    notEmpty_.notify(); // TODO: move outside of lock
  }
  //从队列中取任务
  T take()
  {
    MutexLockGuard lock(mutex_);
    while (queue_.empty())
    {
      notEmpty_.wait();//当队列为空时，就阻塞等待，在put()函数中放完任务以后，会发出条件变量信号
    }
    assert(!queue_.empty());
    T front(queue_.front());
    queue_.pop_front();
    notFull_.notify(); // TODO: move outside of lock
    return front;
  }

  bool empty() const//判断队列是否为空
  {
    MutexLockGuard lock(mutex_);
    return queue_.empty();
  }

  bool full() const//判断是否放满
  {
    MutexLockGuard lock(mutex_);
    return queue_.full();
  }

  size_t size() const//返回队列已有元素个数
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

  size_t capacity() const//返回队列总的容量
  {
    MutexLockGuard lock(mutex_);
    return queue_.capacity();
  }

 private:
  mutable MutexLock          mutex_;
  Condition                  notEmpty_;//定义了两个条件变量类，一个是当任务队列为空时，给取任务的函数使用，
  Condition                  notFull_;//一个是当任务队列为满时，给存任务的函数使用
  boost::circular_buffer<T>  queue_;
  //这是一个boost的环形队列capacity是它的总容量，size是已有的元素个数
};

}

#endif  // MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
