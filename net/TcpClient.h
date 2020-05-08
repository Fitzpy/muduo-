// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/* TcpClient封装了客户端需要用到的所有类，其中包括Eventloop，Connector，TcpConnection，其中Connector类主要是用来与服务端连接的，封装了connect函数
 * TcpConnection则还是一个发送和接受的缓冲区，其实这个和TCPServer很类似。
 * 注意：在Tcpclient中，有Connector和TcpConnection的channel都是绑定的一个文件描述符，就是客户端唯一需要的在创建套接字时产生的文件描述符
 * 但是关注的事件不一样，Connector关注写事件，TcpConnection关注读事件。
 * */
#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/net/TcpConnection.h>

namespace muduo
{
namespace net
{

class Connector;
typedef boost::shared_ptr<Connector> ConnectorPtr;

class TcpClient : boost::noncopyable
{
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const string& host, uint16_t port);
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const string& name);
  ~TcpClient();  // force out-line dtor, for scoped_ptr members.

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const
  {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  bool retry() const;
  void enableRetry() { retry_ = true; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }//绑定在TcpConnection::connectionCallback_函数上，在和客户端建立连接之后和结束连接之前会调用

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }//绑定在TcpConnection::messageCallback_，这个函数在handleWrite函数当中调用了，也可以理解为TcpConnection::channel_写函数的一部分
  //也就是在网络套接字上产生读事件，就会调用

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }//绑定在TcpConnection::writeCompleteCallback_函数上，在handleWrite和sendInLoop写函数中，写完调用的

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);//绑定在Connector::newConnectionCallback_上，在Connector::handleWrite中使用到，也就是一建立连接就会使用这个
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_;	// 用于主动发起连接
  const string name_;		// 名称
  ConnectionCallback connectionCallback_;		// 连接建立回调函数
  MessageCallback messageCallback_;				// 消息到来回调函数
  WriteCompleteCallback writeCompleteCallback_;	// 数据发送完毕回调函数
  bool retry_;   // 重连，是指连接建立之后又断开的时候是否重连
  bool connect_; // atomic
  // always in loop thread
  int nextConnId_;			// name_ + nextConnId_用于标识一个连接
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_; // Connector连接成功以后，得到一个TcpConnection
};

}
}

#endif  // MUDUO_NET_TCPCLIENT_H
