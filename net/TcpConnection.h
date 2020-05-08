// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*我所理解的这个类，主要用来和buffer类一起作为非阻塞IO的一个读取桥梁，其中主要封装的函数是从文件描述符中读取传输的数据到
 *接受缓冲区中，或者把规定数据，或者触发写事件的输出缓冲区的数据写入对应的文件描述符中。
 */
#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/Mutex.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : boost::noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }//获取当前TcpConnection所在的Eventloop
  const string& name() const { return name_; }//
  const InetAddress& localAddress() { return localAddr_; }
  const InetAddress& peerAddress() { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }

  // void send(string&& message); // C++11
  void send(const void* message, size_t len);
  void send(const StringPiece& message);
  // void send(Buffer&& message); // C++11
  void send(Buffer* message);  // this one will swap data
  void shutdown(); // NOT thread safe, no simultaneous calling
  void setTcpNoDelay(bool on);

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const//得到常数值的context_
  { return context_; }

  boost::any* getMutableContext()//得到可以改变的context_
  { return &context_; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }//在handleClose，connectEstablished，connectDestroyed中调用，个人理解这个连接回调函数主要起到
  //显示作用，就是在和连接描述符建立连接或者关闭连接前，显示连接状态的，表明还在连接中

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }//在handleRead函数当中调用了，也可以理解为channel_写函数的一部分

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }//在handleWrite和sendInLoop写函数中，写完调用的

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }//都在sendInLoop中调用了

  Buffer* inputBuffer()
  { return &inputBuffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }//在handleClose函数中调用

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(Timestamp receiveTime);//绑定channel_的读函数
  void handleWrite();//绑定channel_的写函数
  void handleClose();//绑定channel_的关闭函数，同时也在handleRead中调用
  void handleError();////绑定channel_的错误函数
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  void setState(StateE s) { state_ = s; }//设置状态位

  EventLoop* loop_;			// 所属EventLoop
  string name_;				// 连接名
  StateE state_;  // FIXME: use atomic variable
  // we don't expose those classes to client.
  //连接状态
  boost::scoped_ptr<Socket> socket_;
  boost::scoped_ptr<Channel> channel_;
  //channel_在TCPServer中绑定了连接套接字，就是能够实现通信的那个connfd套接字，这个套接字是从Socket::accept函数得到的
  //在Tcpclient绑定的是创建的套接字，因为客户端只需要一个套接字就可以了，这个套接字是从socket()函数中得到的
  InetAddress localAddr_;//当前服务端的地址
  InetAddress peerAddr_;//当前建立连接的客户端地址
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;		// 数据发送完毕回调函数，即所有的用户数据都已拷贝到内核缓冲区时回调该函数
													// outputBuffer_被清空也会回调该函数，可以理解为低水位标回调函数
  HighWaterMarkCallback highWaterMarkCallback_;	    // 高水位标回调函数
  CloseCallback closeCallback_;
  size_t highWaterMark_;		// 高水位标
  Buffer inputBuffer_;			// 应用层接收缓冲区
  Buffer outputBuffer_;			// 应用层发送缓冲区
  boost::any context_;			// 绑定一个未知类型的上下文对象，一般用来放HttpContext类的
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

}
}

#endif  // MUDUO_NET_TCPCONNECTION_H
