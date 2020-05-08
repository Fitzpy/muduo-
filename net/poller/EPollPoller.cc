// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/poller/EPollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
const int kNew = -1;//代表不在epoll队列中，也不在ChannelMap channels_中
const int kAdded = 1;//代表正在epoll队列当中
const int kDeleted = 2;//代表曾经在epoll队列当中过，但是被删除了，现在不在了，但是还是在ChannelMap channels_中的
}

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),//创建一个epoll文件描述符，用来监听所有注册的了事件
    events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller()//关闭epoll文件描述符
{
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)//阻塞等待事件的发生，并且在发生后进行相关的处理
{
  int numEvents = ::epoll_wait(epollfd_,
                               &*events_.begin(),//等价于&events[0],就是传入一个vecotr<struct epoll_event>的首指针进去
                               static_cast<int>(events_.size()),
                               timeoutMs);//numEvents是活跃的文件描述符个数，就是待处理的文件描述符
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size())//如果活跃的文件符个数和存储活跃文件描述符的容量一样，就扩充events_
    {
      events_.resize(events_.size()*2);
    }
  }
  else if (numEvents == 0)//如果timeoutMs设置的是大于0的数，也就是超时时间有效的话，那么过了超时时间并且没有事件发生，就会出现这种情况
  {
    LOG_TRACE << " nothing happended";
  }
  else
  {
    LOG_SYSERR << "EPollPoller::poll()";
  }
  return now;//返回的是事件发生时的时间
}

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const//就是把需要处理的channel放到一个活跃channel列表中
{
  assert(implicit_cast<size_t>(numEvents) <= events_.size());//如果活跃的文件描述符个数大于活跃的文件描述符的容器个数，说明出错了，所以终止
  for (int i = 0; i < numEvents; ++i)//将所有的活跃channel放到activeChannels列表中
  {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);//把产生事件的channel变量拿出来
#ifndef NDEBUG//在调试时会执行下面的代码，否则就直接忽视
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);//判断ChannelMap中key和value的对应关系是否准确
#endif
    channel->set_revents(events_[i].events);//把已经触发的事件写入channel中
    activeChannels->push_back(channel);//把channel放入要处理的channel列表中
  }
}

void EPollPoller::updateChannel(Channel* channel)//根据channel的序号在epoll队列中来删除，增加channel或者改变channel
{
  Poller::assertInLoopThread();//？？？暂时不明白为什么要这么判断，也就是负责epoll_wait的线程和创建eventloop的线程为同一个
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  const int index = channel->index();
  if (index == kNew || index == kDeleted)//如果是完全没在或者曾经在epoll队列中的，就添加到epoll队列中
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {
      assert(channels_.find(fd) == channels_.end());//确保这个channel的文件描述符不在channels_中
      channels_[fd] = channel;//将新添加的fd和channel添加到channels_中
    }
    else // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());//确保这个channel的文件描述符在channels_中
      assert(channels_[fd] == channel);//确保在epoll队列中channel和fd一致
    }
    channel->set_index(kAdded);//修改index为已在队列中
    update(EPOLL_CTL_ADD, channel);
  }
  else//如果是现在就在epoll队列中的，如果没有关注事件了，就暂时删除，如果有关注事件，就修改
  {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());//channels_中是否有这个文件描述符
    assert(channels_[fd] == channel);//channels_中channel和fd是否一致
    assert(index == kAdded);//标志位是否正在队列中
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(Channel* channel)//完全删除channel
{
  Poller::assertInLoopThread();//？？？暂时不明白为什么要这么判断，也就是负责epoll管理的线程和创建eventloop的线程为同一个
  int fd = channel->fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());//channels_中是否有这个文件描述符
  assert(channels_[fd] == channel);//channels_中channel和fd是否一致
  assert(channel->isNoneEvent());//channel中要关注的事件是否为空
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);//标志位必须是kAdded或者kDeleted
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);//从epoll队列中删除这个channel
  }
  channel->set_index(kNew);//设置标志位是kNew，相当于完全删除
}

void EPollPoller::update(int operation, Channel* channel)//主要执行epoll_ctl函数
{
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;//设置epoll_event结构体
  int fd = channel->fd();
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;
    }
    else
    {
      LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
    }
  }
}

