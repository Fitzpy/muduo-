// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/*就是用一个Acceptor类专门用一个channel来创建套接字，绑定，监听等操作*/
#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

namespace muduo
{
namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd,
                                const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }//newConnectionCallback_是在Acceptor::handleRead里面执行的，也就是在acceptChannel_的读事件发生的时候会被调用

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();//可读回调函数，绑定在acceptChannel_的读函数上

  EventLoop* loop_;
  Socket acceptSocket_;//监听套接字
  Channel acceptChannel_;//acceptChannel_和监听套接字acceptSocket_绑定
  NewConnectionCallback newConnectionCallback_;//一旦有新连接发生，执行的回调函数
  bool listenning_;//acceptChannel所处的eventloop是否处于监听状态
  int idleFd_;//用来解决文件描述符过多，引起点平触发不断触发的问题，详见handleRead函数的最后
};

}
}

#endif  // MUDUO_NET_ACCEPTOR_H
