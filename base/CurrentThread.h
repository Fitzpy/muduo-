// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*当前进程的信息*/
#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo
{
namespace CurrentThread
{
  // internal
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread const char* t_threadName;//这三个都是引用了Thread.cc中定义的三个变量
  void cacheTid();

  inline int tid()//获取当前进程tid
  {
    if (t_cachedTid == 0)
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging//获取当前线程tid字符串表现形式
  {
    return t_tidString;
  }

  inline const char* name()//获取线程的名字
  {
    return t_threadName;
  }

  bool isMainThread();//判断当前线程是否是主线程
}
}

#endif
