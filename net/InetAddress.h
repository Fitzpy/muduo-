// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*封装了一个类来初始化sockaddr_in结构体，以及由sockaddr_in结构体得到IP地址和端口的函数*/
#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <muduo/base/copyable.h>
#include <muduo/base/StringPiece.h>

#include <netinet/in.h>

namespace muduo
{
namespace net
{

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress : public muduo::copyable
{
 public:
  /*三个构造函数都是初始化sockaddr_in结构体的
   *第一个构造函数只要端口号
   *第二个构造函数需要ip和端口号
   *第三个构造函数是将一个初始化好的sockaddr_in结构体输入进去，所以第三个构造函数不需要做任何操作
   **/
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  // 仅仅指定port，不指定ip，则ip为INADDR_ANY（即0.0.0.0）
  explicit InetAddress(uint16_t port);

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(const StringPiece& ip, uint16_t port);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  InetAddress(const struct sockaddr_in& addr)
    : addr_(addr)
  { }

  string toIp() const;
  string toIpPort() const;

  // __attribute__ ((deprecated)) 表示该函数是过时的，被淘汰的
  // 这样使用该函数，在编译的时候，会发出警告
  string toHostPort() const __attribute__ ((deprecated))
  { return toIpPort(); }

  // default copy/assignment are Okay

  const struct sockaddr_in& getSockAddrInet() const { return addr_; }//返回sockaddr_in结构体
  void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }//设置sockaddr_in结构体

  uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }//返回32位整型的IP地址
  uint16_t portNetEndian() const { return addr_.sin_port; }//返回端口量

 private:
  struct sockaddr_in addr_;
};

}
}

#endif  // MUDUO_NET_INETADDRESS_H
