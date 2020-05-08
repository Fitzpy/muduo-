// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*1.创建了Eventloop对象的线程是IO线程*/
#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TimerId.h>

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;
///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : boost::noncopyable
{
 public:
  typedef boost::function<void()> Functor;

  EventLoop();
  ~EventLoop();  // force out-line dtor, for scoped_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  void quit();

  ///
  /// Time when poll returns, usually means data arrivial.
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  void runInLoop(const Functor& cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(const Functor& cb);

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId runAt(const Timestamp& time, const TimerCallback& cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  TimerId runAfter(double delay, const TimerCallback& cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  TimerId runEvery(double interval, const TimerCallback& cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void cancel(TimerId timerId);

  // internal usage
  void wakeup();
  void updateChannel(Channel* channel);		// 在Poller中添加或者更新通道
  void removeChannel(Channel* channel);		// 从Poller中移除通道

  void assertInLoopThread()//判断创造EventLoop的线程和当前线程是否是同一个线程
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();//不是同一个线程，就显示出来，并中止程序
    }
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }//判断创造EventLoop的线程和当前线程是否是同一个线程

  bool eventHandling() const { return eventHandling_; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up这个处理读事件函数是wakeupChannel_的读函数，也就是绑定在wakeupChannel_的读函数上的
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;
  
  bool looping_; /* atomic */
  bool quit_; /* atomic */
  bool eventHandling_; /* atomic *///是否正在处理事件的标志符
  bool callingPendingFunctors_; /* atomic *///是否正在处理pendingFunctors_中的函数
  const pid_t threadId_;		// 创造EventLoop对象的线程ID
  Timestamp pollReturnTime_;
  boost::scoped_ptr<Poller> poller_;//poller_指针虽然是Poller类，但是初始化时，是初始化的Poller的子类
  boost::scoped_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;				// 用于eventfd，通过createEventfd创建出来的
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  // scoped_ptr和shared_ptr一样可以自动释放，但是scoped_ptr是独有的，不能共享控制权，也就是一块动态内存只能有一个scoped_ptr指针
  boost::scoped_ptr<Channel> wakeupChannel_;	// wakeupChannel_绑定的文件描述符是eventfd
  //这是一个特殊的channel，这个channel对应的文件描述符是eventfd，一旦使用了EventLoop::wakeup()函数，wakeupFd_描述符就处于可以被读取的状态了
  ChannelList activeChannels_;		// Poller返回的活跃通道
  Channel* currentActiveChannel_;	// 当前正在处理的活跃通道
  MutexLock mutex_;
  std::vector<Functor> pendingFunctors_; // @BuardedBy mutex_//函数指针数组，这个函数指针主要是通过runInLoop函数传入进来的
};

}
}
#endif  // MUDUO_NET_EVENTLOOP_H
