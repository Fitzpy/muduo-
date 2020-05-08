// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Acceptor.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),//设置监听套接字
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))//这个描述符打开一个linux系统的空文件，所有写入的内容都会被丢弃
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();
  ::close(idleFd_);
}

void Acceptor::listen()//开启监听
{
  loop_->assertInLoopThread();
  listenning_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();//将socket套接字挂到eventloop的epoll上，并开启读监听
}

void Acceptor::handleRead()//读的回调函数，一旦socket套接字监听到连接，epoll就会立刻调用回调函数
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(0);//对端的
  //FIXME loop until no more
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0)
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, peerAddr);
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else
  {
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE)//当accept函数出错时，是因为文件描述符太多了
    {
      ::close(idleFd_);//就关闭一个空闲描述符，相当于现在就有一个空的文件描述符位置了
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);//然后把刚才没有接受的描述符接受进来
      ::close(idleFd_);//把这个描述符给关闭，相当于忽略这个请求连接了
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);//重新开启这个空闲描述符
    }//之所以这样，是因为poll使用的是水平触发，如果没有这个if判断，就会一直触发
  }
}

