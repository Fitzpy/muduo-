// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.
/*从linux的/proc目录下获取进程相关信息*/
#ifndef MUDUO_BASE_PROCESSINFO_H
#define MUDUO_BASE_PROCESSINFO_H

#include <muduo/base/Types.h>
#include <muduo/base/Timestamp.h>
#include <vector>

namespace muduo
{

namespace ProcessInfo
{
  pid_t pid();//获取当前进程pid
  string pidString();//获取当前进程pid，并且以字符串形式返回
  uid_t uid();//获取linux系统中的用户ID，其中root是0
  string username();//获取用户名
  uid_t euid();//获取有效用户ID
  Timestamp startTime();//获取当前时间，由timestamp实现

  string hostname();//得到主机名

  /// read /proc/self/status
  string procStatus();//读取/proc/self/status文件中的内容，这个文件主要是有关当前进程的一些信息
  int openedFiles();//读取/proc/self/fd文件目录下的文件名，其实就是所有文件描述符
  int maxOpenFiles();//获取最大文件描述符限制数

  int numThreads();//返回线程数量
  std::vector<pid_t> threads();//获得当前进程的ID
}

}

#endif  // MUDUO_BASE_PROCESSINFO_H
