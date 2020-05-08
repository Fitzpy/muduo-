// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/InetAddress.h>

#include <muduo/net/Endian.h>
#include <muduo/net/SocketsOps.h>

#include <strings.h>  // bzero
#include <netinet/in.h>

#include <boost/static_assert.hpp>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

using namespace muduo;
using namespace muduo::net;

BOOST_STATIC_ASSERT(sizeof(InetAddress) == sizeof(struct sockaddr_in));
//判断这两个结构体是否大小一样，按照man文档的说明应该是一样的

InetAddress::InetAddress(uint16_t port)
{
  bzero(&addr_, sizeof addr_);//void bzero（void *s, int n）就是把s指针指向的前n个字节置零
  addr_.sin_family = AF_INET;//设置IPv4协议
  addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);//设置为所有主机IP都可以被连接，也就是0.0.0.0
  addr_.sin_port = sockets::hostToNetwork16(port);//将端口转换成网络字节序
}

InetAddress::InetAddress(const StringPiece& ip, uint16_t port)
{
  bzero(&addr_, sizeof addr_);
  sockets::fromIpPort(ip.data(), port, &addr_);//根据ip和端口填充sockaddr_in结构体
}

string InetAddress::toIpPort() const//由成员变量sockaddr_in addr_得到"ip:端口"字符串
{
  char buf[32];
  sockets::toIpPort(buf, sizeof buf, addr_);
  return buf;
}

string InetAddress::toIp() const//由成员变量sockaddr_in addr_只得到IP地址
{
  char buf[32];
  sockets::toIp(buf, sizeof buf, addr_);
  return buf;
}

