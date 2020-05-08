// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Exception.h>

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>

using namespace muduo;

Exception::Exception(const char* msg)
  : message_(msg)
{
  fillStackTrace();
}

Exception::Exception(const string& msg)
  : message_(msg)
{
  fillStackTrace();
}

Exception::~Exception() throw ()
{
}

const char* Exception::what() const throw()
{
  return message_.c_str();
}

const char* Exception::stackTrace() const throw()
{
  return stack_.c_str();
}

//取得异常时，栈上的信息
void Exception::fillStackTrace()
{
  const int len = 200;
  void* buffer[len];
  int nptrs = ::backtrace(buffer, len);
  //traceback是捕获异常信息并输出调用栈的，函数原型int backtrace(void **buffer, int size);
  //相当于buf指针数组中的每一个元素都是一个栈中的地址信息
  char** strings = ::backtrace_symbols(buffer, nptrs);
  //backtrace_symbols就是把栈中的地址信息翻译成一串字符串

  //整理成一个string
  if (strings)
  {
    for (int i = 0; i < nptrs; ++i)
    {
      // TODO demangle funcion name with abi::__cxa_demangle
      //stack_.append(strings[i]);
	  stack_.append(demangle(strings[i]));
      stack_.push_back('\n');
    }
    free(strings);//是在backtrace_symbols中malloc的空间，所以要释放
  }
}

//通过backtrace_symbols得到的栈信息进行进一步处理，得到人可以看懂的信息（暂不细究）
string Exception::demangle(const char* symbol)
{
  size_t size;
  int status;
  char temp[128];
  char* demangled;
  //first, try to demangle a c++ name
  if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
    if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
      string result(demangled);
      free(demangled);
      return result;
    }
  }
  //if that didn't work, try to get a regular c symbol
  if (1 == sscanf(symbol, "%127s", temp)) {
    return temp;
  }
 
  //if all else fails, just return the symbol
  return symbol;
}
