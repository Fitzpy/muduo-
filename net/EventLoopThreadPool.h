// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/*EventLoop池类，其中线程池中有多个EventLoopThread，每一个EventLoopThread可以创建一个拥有EventLoop的IO线程*/
#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
 public:
  typedef boost::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThreadPool(EventLoop* baseLoop);
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }//设置线程池中的线程数
  void start(const ThreadInitCallback& cb = ThreadInitCallback());//这个cb是赋值给EventLoopThread::callback_
  EventLoop* getNextLoop();//按照轮用的机制，拿出一个eventloop出来

 private:

  EventLoop* baseLoop_;	// 与Acceptor所属EventLoop相同
  bool started_;
  int numThreads_;		// 线程数
  int next_;			// 新连接到来，所选择的EventLoop对象下标
  boost::ptr_vector<EventLoopThread> threads_;		// IO线程列表
  std::vector<EventLoop*> loops_;					// EventLoop列表
};

}
}

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H
