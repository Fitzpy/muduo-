// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>
/*个人理解：channel是一个具体来处理事件的类，它与eventloop关系紧密，主要是根据事件宏定义来调用对应的回调函数
 *主要的事件有三种，读事件，写事件和结束事件
 **/
namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;
  typedef boost::function<void(Timestamp)> ReadEventCallback;//读事件的回调函数中必须有参数Timestamp

  Channel(EventLoop* loop, int fd);//一个channel要绑定一个EventLoop和一个文件描述符，但channel无权操作fd
  ~Channel();

  void handleEvent(Timestamp receiveTime);//处理事件
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }//设置四种回调函数

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const boost::shared_ptr<void>&);//将一个shared_ptr指针的值赋给tie_

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }//判断事件是否为0，也就是没有关注的事件

  void enableReading() { events_ |= kReadEvent; update(); }//设置读事件，并将当前channel加入到poll队列当中
  // void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }//设置写事件，并将当前channel加入到poll队列当中
  void disableWriting() { events_ &= ~kWriteEvent; update(); }//关闭写事件，并将当前channel加入到poll队列当中
  void disableAll() { events_ = kNoneEvent; update(); }//关闭所有事件，并暂时删除当前channel
  bool isWriting() const { return events_ & kWriteEvent; }//是否关注写事件

  // for Poller
  int index() { return index_; }//返回序号
  void set_index(int idx) { index_ = idx; }//设置序号

  // for debug
  string reventsToString() const;

  void doNotLogHup() { logHup_ = false; }//把挂起标志位置false

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;			// 所属EventLoop
  const int  fd_;			// 文件描述符，但不负责关闭该文件描述符
  int        events_;		// 需要epoll关注的事件
  int        revents_;		// poll/epoll wait返回的需要处理的事件
  int        index_;		// used by Poller.表示在epoll队列中的状态：1.正在队列中2.曾经在队列中3.从来没在队列中
  bool       logHup_;		// for POLLHUP是否被挂起

  boost::weak_ptr<void> tie_;//保证channel所在的类
  bool tied_;
  bool eventHandling_;		// 是否处于处理事件中
  ReadEventCallback readCallback_;//当文件描述符产生读事件时，最后调用的读函数，我将它命名为channel的读函数
  EventCallback writeCallback_;//当文件描述符产生写事件时，最后调用的写函数，我将它命名为channel的写函数
  EventCallback closeCallback_;//当文件描述符产生关闭事件时，最后调用的关闭函数，我将它命名为channel的关闭函数
  EventCallback errorCallback_;//当文件描述符产生错误事件时，最后调用的错误函数,我将它命名为channel的错误函数
};

}
}
#endif  // MUDUO_NET_CHANNEL_H
