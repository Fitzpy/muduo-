#include <muduo/base/LogStream.h>

#include <algorithm>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::detail;

// 忽略-Wtype-limits
// 关于diagnostic更多信息，可以查看http://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
#pragma GCC diagnostic ignored "-Wtype-limits"
//#pragma GCC diagnostic error "-Wtype-limits"
namespace muduo
{
namespace detail
{

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
BOOST_STATIC_ASSERT(sizeof(digits) == 20);

const char digitsHex[] = "0123456789ABCDEF";
BOOST_STATIC_ASSERT(sizeof digitsHex == 17);

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)//把整数value变为字符串
{
  T i = value;
  char* p = buf;

  do
  {
    int lsd = static_cast<int>(i % 10);
    i /= 10;
    *p++ = zero[lsd];//通过digits数组取数值对应的字符串
  } while (i != 0);//每次循环都取最后一位，比如value=123，第一次取3，第二次取2，第三次取1

  if (value < 0)
  {
    *p++ = '-';
  }
  *p = '\0';
  std::reverse(buf, p);//因为之前是反转的，所以需要反过来

  return p - buf;
}

// uintptr_t对于32平台来说就是unsigned int, 对于64位平台来说unsigned long int
size_t convertHex(char buf[], uintptr_t value)//把指针存储的地址值变为字符串存进buf中
{
  uintptr_t i = value;
  char* p = buf;

  do
  {
    int lsd = i % 16;
    i /= 16;
    *p++ = digitsHex[lsd];
  } while (i != 0);

  *p = '\0';
  std::reverse(buf, p);

  return p - buf;
}

}
}

template<int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
  *cur_ = '\0';
  return data_;
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

void LogStream::staticCheck()//用于检查一些类型的大小，BOOST_STATIC_ASSERT表示在编译的时候就检查了，不会在编译的时候去检查
{
  BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10);//返回double在十进制表示下可以表示的最大位数，因为不同平台可能设置不一样
  BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10);
  BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10);
  BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10);
}

template<typename T>
void LogStream::formatInteger(T v)//将整数转换成字符串并放入buffer_缓存中
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    size_t len = convert(buffer_.current(), v);
    buffer_.add(len);
  }
}

LogStream& LogStream::operator<<(short v)//short类型是转换成对应的int类型，然后再处理
{
  *this << static_cast<int>(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
  *this << static_cast<unsigned int>(v);
  return *this;
}

LogStream& LogStream::operator<<(int v)//将int类型的整数转换成字符串存入buffer_缓冲区。其他整型直接调用formatInteger函数即可
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
  formatInteger(v);
  return *this;
}

LogStream& LogStream::operator<<(const void* p)//把p指针存储的地址值存入buffer_缓冲区中
{
  uintptr_t v = reinterpret_cast<uintptr_t>(p);
  if (buffer_.avail() >= kMaxNumericSize)
  {
    char* buf = buffer_.current();
    buf[0] = '0';
    buf[1] = 'x';
    size_t len = convertHex(buf+2, v);
    buffer_.add(len+2);
  }
  return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
  if (buffer_.avail() >= kMaxNumericSize)
  {
    int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
    buffer_.add(len);
  }
  return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val)//按照fmt的格式将val编码成字符串
{
  // 判断T是否算术类型
  BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);

  length_ = snprintf(buf_, sizeof buf_, fmt, val);
  assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations
//模板的特化，就是只有这些数据类型可以作为模板的参数

template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);
