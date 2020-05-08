// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
  : loop_(loop),
    fd_(fd__),
    events_(0),
    revents_(0),
    index_(-1),//就是kNew
    logHup_(true),
    tied_(false),
    eventHandling_(false)
{
}

Channel::~Channel()
{
  assert(!eventHandling_);
}

void Channel::tie(const boost::shared_ptr<void>& obj)//给tie_指针赋值，tie_指针是一个weak_ptr指针，但是给weak_ptr指针赋值的一定是一个shared_ptr指针
{
  tie_ = obj;
  tied_ = true;
}

void Channel::update()//把当前的channel加入到poll队列当中
{
  loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
// 从EventLoop中移除这个channel
void Channel::remove()
{
  assert(isNoneEvent());
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)//Timestamp主要用于读事件的回调函数
{
  boost::shared_ptr<void> guard;
  if (tied_)
  {
    guard = tie_.lock();//提升tie_为shared_ptr，如果提升成功，说明指向一个存在的对象
    if (guard)
    {
      LOG_TRACE << "[6] usecount=" << guard.use_count();
      handleEventWithGuard(receiveTime);
	  LOG_TRACE << "[12] usecount=" << guard.use_count();
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
//暂时理解：查看epoll/或者poll返回的具体是什么事件，并根据事件的类型进行相应的处理
{
  eventHandling_ = true;
  /*
  if (revents_ & POLLHUP)
  {
	  LOG_TRACE << "1111111111111111";
  }
  if (revents_ & POLLIN)
  {
	  LOG_TRACE << "2222222222222222";
  }
  */
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN))//当事件为挂起并没有可读事件时
  {
    if (logHup_)
    {
      LOG_WARN << "Channel::handle_event() POLLHUP";
    }
    if (closeCallback_) closeCallback_();
  }

  if (revents_ & POLLNVAL)//描述字不是一个打开的文件描述符
  {
    LOG_WARN << "Channel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL))//发生错误或者描述符不可打开
  {
    if (errorCallback_) errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))//关于读的事件
  {
    if (readCallback_) readCallback_(receiveTime);
  }
  if (revents_ & POLLOUT)//关于写的事件
  {
    if (writeCallback_) writeCallback_();
  }
  eventHandling_ = false;
}

string Channel::reventsToString() const//把事件编写成一个string
{
  std::ostringstream oss;
  oss << fd_ << ": ";
  if (revents_ & POLLIN)
    oss << "IN ";
  if (revents_ & POLLPRI)
    oss << "PRI ";
  if (revents_ & POLLOUT)
    oss << "OUT ";
  if (revents_ & POLLHUP)
    oss << "HUP ";
  if (revents_ & POLLRDHUP)
    oss << "RDHUP ";
  if (revents_ & POLLERR)
    oss << "ERR ";
  if (revents_ & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}
