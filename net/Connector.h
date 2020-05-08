// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/*封装了客户端的connect函数，按理来说客户端是不需要使用epoll的，但是这里也使用了，只是为了测试连接是否成功，测试结束以后就从
 *epoll队列中拿下来了，并且如果测试连接失败，还会不断重新连接*/
#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include <muduo/net/InetAddress.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;

// 主动发起连接，带有自动重连功能
class Connector : boost::noncopyable,
                  public boost::enable_shared_from_this<Connector>
{
 public:
  typedef boost::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }//该函数在channel_的写函数中调用

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;			// 30秒，最大重连延迟时间
  static const int kInitRetryDelayMs = 500;				// 0.5秒，初始状态，连接不上，0.5秒后重连

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();//绑定为channel_的写函数
  void handleError();//绑定为channel_的错误函数
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;			// 所属EventLoop
  InetAddress serverAddr_;	// 服务器端地址
  bool connect_; // atomic//是否真正在连接，或者执行连接上的回调函数的标志
  States state_;  // FIXME: use atomic variable客户端连接状态标志位
  boost::scoped_ptr<Channel> channel_;	// Channel绑定客户端的socket套接字，客户端的网络套接字只有这一个
  NewConnectionCallback newConnectionCallback_;		// 连接成功回调函数，
  int retryDelayMs_;		// 重连延迟时间（单位：毫秒）
};

}
}

#endif  // MUDUO_NET_CONNECTOR_H
