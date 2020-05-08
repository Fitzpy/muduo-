#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H
/* FixedBuffer封装了一个缓冲区，及对缓冲区的操作函数
 * LogStream是一个输入输出流，将相应的数据类型，主要是字符串和数值类型，存入缓存区，主要是重载<<符号
 * ？？？Fmt就是将值按照fmt字符的格式，组成相应的字符串，但是有很多限制，只能存32字节大小的数据，而且只能存一个value
 * 看它的调用是用来处理小数的。
 * */
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#ifndef MUDUO_STD_STRING
#include <string>
#endif
#include <boost/noncopyable.hpp>

namespace muduo
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;

// SIZE为非类型参数，这个参数是常数，就相当于一个传入的常数参数一样
template<int SIZE>
class FixedBuffer : boost::noncopyable
{
 public:
  FixedBuffer()
    : cur_(data_)
  {
    setCookie(cookieStart);
  }

  ~FixedBuffer()
  {
    setCookie(cookieEnd);
  }

  void append(const char* /*restrict*/ buf, size_t len)//把buf字符串存入缓存中
  {
    // FIXME: append partially
    if (implicit_cast<size_t>(avail()) > len)
    {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }//返回整个缓冲区
  int length() const { return static_cast<int>(cur_ - data_); }//返回已存的数据长度

  // write to data_ directly
  char* current() { return cur_; }//返回可用缓存空间的首地址
  int avail() const { return static_cast<int>(end() - cur_); }//获得可以使用的缓存区空间
  void add(size_t len) { cur_ += len; }//减少len长度的可用空间

  void reset() { cur_ = data_; }//释放所有的已存数据
  void bzero() { ::bzero(data_, sizeof data_); }//把data数组清零

  // for used by GDB
  const char* debugString();//就是把data首地址变为\0，也就是将这个清除
  void setCookie(void (*cookie)()) { cookie_ = cookie; }//设置cookie_函数
  // for used by unit test
  string asString() const { return string(data_, length()); }//把data中char数组转换成string类型

 private:
  const char* end() const { return data_ + sizeof data_; }//得到指向data_数组结尾的指针
  // Must be outline function for cookies.
  static void cookieStart();//空函数
  static void cookieEnd();//空函数

  void (*cookie_)();
  char data_[SIZE];//缓冲区
  char* cur_;//指向缓冲区空闲空间的首地址
};

}

class LogStream : boost::noncopyable
{
  typedef LogStream self;
 public:
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;

  self& operator<<(bool v)
  {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);

  self& operator<<(const void*);

  self& operator<<(float v)//把float类型转换成double类型，再用<<符号
  {
    *this << static_cast<double>(v);
    return *this;
  }
  self& operator<<(double);
  // self& operator<<(long double);

  self& operator<<(char v)//单个字符直接添加进buffer_缓冲区中
  {
    buffer_.append(&v, 1);
    return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<(const char* v)//添加一个字符串进缓冲区中
  {
    buffer_.append(v, strlen(v));
    return *this;
  }

  self& operator<<(const string& v)//添加string进缓冲区中
  {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

#ifndef MUDUO_STD_STRING
  self& operator<<(const std::string& v)
  {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }
#endif

  self& operator<<(const StringPiece& v)//添加StringPiece进缓冲区中
  {
    buffer_.append(v.data(), v.size());
    return *this;
  }

  void append(const char* data, int len) { buffer_.append(data, len); }//就是调用缓冲区类的append函数
  const Buffer& buffer() const { return buffer_; }//返回缓冲区类
  void resetBuffer() { buffer_.reset(); }//重置缓冲区

 private:
  void staticCheck();

  template<typename T>
  void formatInteger(T);

  Buffer buffer_;

  static const int kMaxNumericSize = 32;
};

class Fmt // : boost::noncopyable
{
 public:
  template<typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s;
}

}
#endif  // MUDUO_BASE_LOGSTREAM_H

