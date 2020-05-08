// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/* 定时器管理类，其中timer类就是TimerQueue需要管理的元素，而timerId就是一个简单的timer封装，避免销毁和创建操作
 * 但是要注意的是timer并没有自己计时的功能，所以需要依靠timerfd这个系统函数统一计时
 * timerfd是一个系统计时函数，当所设置的时间到了，会通过timerfd这个文件描述符进行提示通信，而其他计时函数可能是通过信号量，或者
 * 其他方式，但是都没有文件描述符好，并且也可以用timerfd监听，具体原因可以查看一下博客的网络库定时器实现
 * 如何使用timerfd来为所有的计时器计时：timerfd每次都设置在计时器列表中到期时间最近的那个到期时间，这样timerfd到期以后，也就是最近的那个计时器到期
 * 所以每次都是手动重置timerfd的计时时间，为最近的计时器到期时间
 * ？timestamp::now获得的时间是从1960年1月1日开始的，但是timerfd据说是从系统开机的那一刻开始的，那么在设置timefd时时间不统一怎么办
 * 注意在timerfd设置延时的时候，使用的是相对时间，所以无所谓最终时间是多少，只要相对时间没问题就好了
 * ？重置timerfd导致延时怎么办
 * ？关于线程执行，究竟哪些函数靠IO线程来执行
 * 
 * */
#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Channel.h>

namespace muduo
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : boost::noncopyable
{
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  // 一定是线程安全的，可以跨线程调用。通常情况下被其它线程调用。
  TimerId addTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);

  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // unique_ptr是C++ 11标准的一个独享所有权的智能指针
  // 无法得到指向同一对象的两个unique_ptr指针
  // 但可以进行移动构造与移动赋值操作，即所有权可以移动到另一个对象（而非拷贝构造）
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;//set中存储的是pair类型，那么默认先按照pair的第一个元素排序，如果相同，再按照第二个元素排序。
  //所以这两种set都是存放定时器的列表，但是一个根据定时器的到时时间来存储，
  //一个根据定时器地址来存储，但是存储的定时器都是同一个，目的是为了区分同一到期时间的定时器？？？

  // 以下成员函数只可能在其所属的I/O线程中调用，因而不必加锁。
  // 服务器性能杀手之一是锁竞争，所以要尽可能少用锁
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void handleRead();//timerfdChannel_的读函数
  // move out all expired timers
  // 返回超时的定时器列表
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;		// 所属EventLoop
  const int timerfd_;
  //过一段事件，就筛选一次，看看TimerList中有多少定时器到时间了，就处理一下，但是这样延迟很高，不太理解
  Channel timerfdChannel_;//与timefd绑定
  // Timer list sorted by expiration
  TimerList timers_;	// timers_是按到期时间排序，也是存放未到时间的定时器

  // for cancel()
  // timers_与activeTimers_保存的是相同的数据
  // timers_是按到期时间排序，activeTimers_是按对象地址排序
  ActiveTimerSet activeTimers_;//还未到时间的定时器,这里面存放的定时器是和timers_一样的，只是顺序不同
  bool callingExpiredTimers_; /* atomic *///是否在处理过期定时器的标志
  ActiveTimerSet cancelingTimers_;	// 保存的是被取消的定时器//用这个列表的作用是，当出现一个循环的计时器被取消时，就要通过reset函数中对
  //ActiveTimerSet列表来暂停对这个计时器的重置
};

}
}
#endif  // MUDUO_NET_TIMERQUEUE_H
