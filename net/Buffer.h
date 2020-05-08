// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*设计了缓冲区类，每个类都可以根据接受数据的大小自动扩展缓冲区*/
#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include <muduo/base/copyable.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>

#include <muduo/net/Endian.h>

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>
//#include <unistd.h>  // ssize_t

namespace muduo
{
namespace net
{

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer : public muduo::copyable
{ 
 public:
  static const size_t kCheapPrepend = 8;//默认预留8个字节，这8个字节是不会存放数据的
  static const size_t kInitialSize = 1024;//初始大小

  Buffer()
    : buffer_(kCheapPrepend + kInitialSize),//刚开始默认创建一个1024+8的缓冲区
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
  {
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
  }

  // default copy-ctor, dtor and assignment are fine

  void swap(Buffer& rhs)//交换buffer_和rhs
  {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  size_t readableBytes() const//返回可读大小
  { return writerIndex_ - readerIndex_; }

  size_t writableBytes() const//返回可写大小
  { return buffer_.size() - writerIndex_; }

  size_t prependableBytes() const//返回预留区域+已经读完的数据的大小
  { return readerIndex_; }

  const char* peek() const//返回读的下标
  { return begin() + readerIndex_; }

  const char* findCRLF() const//在可读数据中查找结尾符
  {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char* findCRLF(const char* start) const//从指定的start位置开始，到写位置处中查找结尾符
  {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  // retrieve returns void, to prevent
  // string str(retrieve(readableBytes()), readableBytes());
  // the evaluation of two functions are unspecified
  void retrieve(size_t len)//读完数据以后处理读指针的位置
  {
    assert(len <= readableBytes());
    if (len < readableBytes())//仍有待读的数据
    {
      readerIndex_ += len;
    }
    else
    {
      retrieveAll();//数据都读完了
    }
  }

  void retrieveUntil(const char* end)//读取从readerIndex_读指针到end位置所有的数据
  {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }
  //处理读写指针
  void retrieveInt32()
  {
    retrieve(sizeof(int32_t));
  }

  void retrieveInt16()
  {
    retrieve(sizeof(int16_t));
  }

  void retrieveInt8()
  {
    retrieve(sizeof(int8_t));
  }

  void retrieveAll()//所有数据都读完了，读写指针都复位
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  string retrieveAllAsString()//读出所有未读的数据，并用string类型存放
  {
    return retrieveAsString(readableBytes());;
  }

  string retrieveAsString(size_t len)//指定读出多少字节的数据，并且以string格式存放
  {
    assert(len <= readableBytes());
    string result(peek(), len);
    retrieve(len);
    return result;
  }

  StringPiece toStringPiece() const//相当于把需要读首地址指针和需要读的数据大小封装到StringPiece中
  {
    return StringPiece(peek(), static_cast<int>(readableBytes()));
  }

  void append(const StringPiece& str)
  {
    append(str.data(), str.size());
  }

  void append(const char* /*restrict*/ data, size_t len)//data是要添加的字符串指针，添加字符串的长度
  {
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    hasWritten(len);
  }

  void append(const void* /*restrict*/ data, size_t len)
  {
    append(static_cast<const char*>(data), len);
  }

  // 确保缓冲区可写空间>=len，如果不足则扩充
  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)//如果可写空间<len
    {
      makeSpace(len);//创建空间
    }
    assert(writableBytes() >= len);
  }

  char* beginWrite()//返回可以写的位置的指针
  { return begin() + writerIndex_; }

  const char* beginWrite() const//返回可以写的位置的常数指针
  { return begin() + writerIndex_; }

  void hasWritten(size_t len)//在写完len个指针之后改变writerIndex_的值到对应位置
  { writerIndex_ += len; }

  ///
  /// Append int32_t using network endian
  ///
  //添加数据的封装
  void appendInt32(int32_t x)
  {
    int32_t be32 = sockets::hostToNetwork32(x);
    append(&be32, sizeof be32);
  }

  void appendInt16(int16_t x)
  {
    int16_t be16 = sockets::hostToNetwork16(x);
    append(&be16, sizeof be16);
  }

  void appendInt8(int8_t x)
  {
    append(&x, sizeof x);
  }

  ///
  /// Read int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  //读数据的封装，包括读数据和处理读指针
  int32_t readInt32()
  {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
  }

  int16_t readInt16()
  {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
  }

  int8_t readInt8()
  {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
  }

  ///
  /// Peek int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t peekInt32() const//读32位数据
  {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof be32);
    return sockets::networkToHost32(be32);
  }

  int16_t peekInt16() const//读16位数据
  {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof be16);
    return sockets::networkToHost16(be16);
  }

  int8_t peekInt8() const//读八位数据
  {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
  }

  ///
  /// Prepend int32_t using network endian
  ///
  void prependInt32(int32_t x)//向已读区域中放入32位数据
  {
    int32_t be32 = sockets::hostToNetwork32(x);
    prepend(&be32, sizeof be32);
  }

  void prependInt16(int16_t x)//向已读区域中放入16位数据
  {
    int16_t be16 = sockets::hostToNetwork16(x);//主机向网络字节序转换
    prepend(&be16, sizeof be16);
  }

  void prependInt8(int8_t x)//向已读区域中放入8位数据
  {
    prepend(&x, sizeof x);
  }

  void prepend(const void* /*restrict*/ data, size_t len)//在已读区域中放置数据，并且这个数据是紧靠未读数据的，然后将读指针移到刚放置的区域开头
  {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
  }

  // 收缩，保留reserve个字节+buffer_中可读的字节
  void shrink(size_t reserve)
  {
    // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
    Buffer other;
    other.ensureWritableBytes(readableBytes()+reserve);//为了other创造reserve个字节+buffer_中可读的字节
    other.append(toStringPiece());//这就是把可读的内容拷贝到other数组中去
    swap(other);//将buffer_和other交换，相当于就收缩了buffer_数组
  }

  /// Read data directly into buffer.
  ///
  /// It may implement with readv(2)
  /// @return result of read(2), @c errno is saved
  ssize_t readFd(int fd, int* savedErrno);//从文件描述符中把数据读到缓冲区中

 private:

  char* begin()//得到缓冲区的首地址
  { return &*buffer_.begin(); }

  const char* begin() const//得到buffer_的头指针
  { return &*buffer_.begin(); }

  void makeSpace(size_t len)//为len个字节创建空间
  {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)//确保可写空间+预留空间+已读空间<待写空间+预留空间
    {
      // FIXME: move readable data
      buffer_.resize(writerIndex_+len);//扩充vector数组
    }
    else//如果可以依靠内部腾挪就装得下
    {
      // move readable data to the front, make space inside buffer
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();//返回需要读的字节数
      std::copy(begin()+readerIndex_,
                begin()+writerIndex_,
                begin()+kCheapPrepend);//把从begin()+readerIndex_位置开始，到begin()+writerIndex_位置的数据拷贝到kCheapPrepend中
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;//重置读位置和写位置的值
      assert(readable == readableBytes());
    }
  }

 private:
  std::vector<char> buffer_;	// vector用于替代固定大小数组，就相当于字符串数组
  size_t readerIndex_;			// 读位置
  size_t writerIndex_;			// 写位置

  static const char kCRLF[];	// "\r\n"柔性数组，我觉得不是柔性数组，因为这是一个静态常量数组，其实是在源文件中对它进行了赋值，而不是在这里
};

}
}

#endif  // MUDUO_NET_BUFFER_H
