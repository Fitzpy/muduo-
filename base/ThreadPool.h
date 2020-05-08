// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*线程池类*/
#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

class ThreadPool : boost::noncopyable
{
 public:
  typedef boost::function<void ()> Task;

  explicit ThreadPool(const string& name = string());//等号是表示默认的意思，如果这个参数没有填，就是默认为空字符
  ~ThreadPool();

  void start(int numThreads);
  void stop();

  void run(const Task& f);

 private:
  void runInThread();
  Task take();

  MutexLock mutex_;
  Condition cond_;
  string name_;
  boost::ptr_vector<muduo::Thread> threads_;//boost::ptr_vector 专门用于动态分配的对象
  //是动态内存Thread类的指针，threads_数组中就是一个元素代表一个线程
  std::deque<Task> queue_;//deque容器类似与vector，但是可以高效得在头部进行插入或者删除
  //queue存储的是任务队列
  bool running_;//线程池运行标志位
};

}

#endif
