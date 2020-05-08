// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/*1.这个类主要利用epoll函数，封装了epoll三个函数，
 *2.其中epoll_event.data是一个指向channel类的指针
 *这里可以等价理解为channel就是epoll_event，用于在epoll队列中注册，删除，更改的结构体
 *因为文件描述符fd，Channel，以及epoll_event结构体（只有需要添加到epoll上时才有epoll_event结构体）
 *三个都是一一对应的关系Channel.fd应该等于fd，epoll_event.data应该等于&Channel
 *如果不添加到epoll队列中，Channel和fd一一对应，就没有epoll_event结构体了
 *3.从epoll队列中删除有两种删除方法，
 *第一种暂时删除，就是从epoll队列中删除，并且把标志位置为kDeleted，但是并不从ChannelMap channels_中删除
 *第二种是完全删除，从epoll队列中删除，并且从ChannelMap channels_中也删除，最后把标志位置kNew
 *可以理解为ChannelMap channels_的作用就是：暂时不需要的，就从epoll队列中删除，但是在channels_中保留信息，类似与挂起，这样
 *下次再使用这个channel时，只需要添加到epoll队列中即可。而完全删除，就把channels_中也删除。
 */
#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

struct epoll_event;

namespace muduo
{
namespace net
{

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  static const int kInitEventListSize = 16;

  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;
  typedef std::map<int, Channel*> ChannelMap;

  int epollfd_;//epoll监视的文件描述符
  EventList events_;//用来存储活跃文件描述符的epoll_event结构体数组
  ChannelMap channels_;//记录标志符是kAdded或者kDeleted的channel和fd
};

}
}
#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H
