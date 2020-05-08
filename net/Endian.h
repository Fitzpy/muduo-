// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*字节序转换函数封装*/
#ifndef MUDUO_NET_ENDIAN_H
#define MUDUO_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace muduo
{
namespace net
{
namespace sockets
{

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#if __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
/*32位用来转换ip地址，16位用来转换端口号*/
inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);//64位的主机字节向大端字节转换
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htobe32(host32);//32位的主机字节向大端字节转换
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htobe16(host16);//16位的主机字节向大端字节转换
}

inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);//64位的大端字节向主机字节转换
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return be32toh(net32);//32位的大端字节向主机字节转换
}

inline uint16_t networkToHost16(uint16_t net16)
{
  return be16toh(net16);//16位的大端字节向主机字节转换
}
#if __GNUC_MINOR__ >= 6
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic error "-Wconversion"
#pragma GCC diagnostic error "-Wold-style-cast"
#endif


}
}
}

#endif  // MUDUO_NET_ENDIAN_H
