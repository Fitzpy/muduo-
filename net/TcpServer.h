// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/* 这个类相当于把TcpConnection以及Accept类整合起来，完全能够实现Tcp通信，也就是socket函数都实现了
 * 总结一下整个TCP通信过程：
 * 一个TcpServer类中，有Acceptor，EventLoopThreadPool各一个，以及多个TcpConnection类的指针，
 * 在TcpServer类的启动函数中，先开启EventLoopThreadPool线程池，然后将Acceptor监听函数放入eventloop中去执行
 * 在TcpServer类的构造函数中，就已经把一个成功连接的回调函数绑定在Acceptor类的连接回调函数中，如果Acceptor监听
 * 到有连接进来，先调监听socket描述符的回调函数，把这个连接accept进来，然后再调用newConnectionCallback_函数
 * 来处理连接，每个连接都有一个对应的TcpConnection类来作为缓冲区
 * */
#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/Types.h>
#include <muduo/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : boost::noncopyable
{
 public:
  typedef boost::function<void(EventLoop*)> ThreadInitCallback;

  //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
  ~TcpServer();  // force out-line dtor, for scoped_ptr members.

  const string& hostport() const { return hostport_; }
  const string& name() const { return name_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback& cb)
  { threadInitCallback_ = cb; }//这个函数会作为EventLoopThreadPool::start的入口参数

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  // 设置连接到来或者连接关闭回调函数,这个函数指针会赋值给TcpConnection::connectionCallback_函数，就是在连接建立之后，和连接断开之前会调用
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  //  设置消息到来回调函数，这个函数指针在TcpConnection::handleread函数中调用，也就是TcpConnection的Channel的读函数的一部分
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  /// 在发送完消息以后调用，这个函数指针会赋值给TcpConnection::writeCompleteCallback_函数
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }


 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);//这个函数会赋值给Acceptor::newConnectionCallback_，在新连接建立以后调用
  /// Thread safe.
  /// 会赋值给TcpConnection::closeCallback_函数，也就是当连接描述符关闭以后调用这个
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop，在上面这个函数removeConnection中调用
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  typedef std::map<string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;  // the acceptor loop
  const string hostport_;		// 服务的ip:端口
  const string name_;			// 服务名
  boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  boost::scoped_ptr<EventLoopThreadPool> threadPool_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;		// 数据发送完毕，会回调此函数
  ThreadInitCallback threadInitCallback_;	// IO线程池中的线程在进入事件循环前，会回调用此函数
  bool started_;
  // always in loop thread
  int nextConnId_;				// 下一个连接ID,每次增加一个就加1
  ConnectionMap connections_;	// 连接列表
};

}
}

#endif  // MUDUO_NET_TCPSERVER_H
