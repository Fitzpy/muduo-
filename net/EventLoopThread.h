// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*其实EventLoopThread类就是对创建IO线程的封装，但是注意IO线程并不是这个类对象所在的线程，而是这个线程创建的子线程
 */
#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
namespace net
{

class EventLoop;

class EventLoopThread : boost::noncopyable
{
 public:
  typedef boost::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());//这里cb = ThreadInitCallback()的意思是
  //默认值是ThreadInitCallback()，而ThreadInitCallback()其实就是boost::function<void(EventLoop*)>类的空列表的初始化参数
  //我觉得就是一个空的指针函数指针类，其实我觉得直接写NULL也可以
  ~EventLoopThread();
  EventLoop* startLoop();	// 启动线程，该线程就成为了IO线程

 private:
  void threadFunc();		// 线程函数

  EventLoop* loop_;			// loop_指针指向一个EventLoop对象
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
  ThreadInitCallback callback_;		// 该函数在EventLoop::loop事件循环之前被调用，就是在threadFunc函数中，创建完子线程之后，调用的，可以自定义
};

}
}

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

