// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <muduo/base/ProcessInfo.h>
#include <muduo/base/FileUtil.h>

#include <algorithm>

#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h> // snprintf
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

namespace muduo
{
namespace detail
{
__thread int t_numOpenedFiles = 0;
int fdDirFilter(const struct dirent* d)
{
  //检测文件名第一个字符是否是十进制数，因为这个是用于筛选目录/proc/self/fd或者/proc/self/task中的文件，都是以数字开头的，
  //所以相当于筛选出所有的文件描述符
  if (::isdigit(d->d_name[0]))
  {
    ++t_numOpenedFiles;
  }
  return 0;
}

__thread std::vector<pid_t>* t_pids = NULL;
int taskDirFilter(const struct dirent* d)
{
  if (::isdigit(d->d_name[0]))
  {
    t_pids->push_back(atoi(d->d_name));
  }
  return 0;
}

int scanDir(const char *dirpath, int (*filter)(const struct dirent *))
{
  struct dirent** namelist = NULL;
  int result = ::scandir(dirpath, &namelist, filter, alphasort);//将目录中满足filter函数过滤的文件结构，并返回到namelist中
  assert(namelist == NULL);
  return result;
}

Timestamp g_startTime = Timestamp::now();
}
}

using namespace muduo;
using namespace muduo::detail;

pid_t ProcessInfo::pid()
{
  return ::getpid();
}

string ProcessInfo::pidString()
{
  char buf[32];
  snprintf(buf, sizeof buf, "%d", pid());
  return buf;
}

uid_t ProcessInfo::uid()
{
  return ::getuid();
}

string ProcessInfo::username()
{
  struct passwd pwd;
  struct passwd* result = NULL;
  char buf[8192];
  const char* name = "unknownuser";

  getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
  if (result)
  {
    name = pwd.pw_name;
  }
  return name;
}

uid_t ProcessInfo::euid()
{
  return ::geteuid();
}

Timestamp ProcessInfo::startTime()
{
  return g_startTime;
}

string ProcessInfo::hostname()//得到主机名称，如果没有，就是unknownhost
{
  char buf[64] = "unknownhost";
  buf[sizeof(buf)-1] = '\0';
  ::gethostname(buf, sizeof buf);
  return buf;
}

string ProcessInfo::procStatus()
{
  string result;
  FileUtil::readFile("/proc/self/status", 65536, &result);

  return result;
}

int ProcessInfo::openedFiles()
{
  t_numOpenedFiles = 0;
  scanDir("/proc/self/fd", fdDirFilter);//在fdDirFilter函数中会改变
  return t_numOpenedFiles;//该进程已使用的文件描述符数目
}

int ProcessInfo::maxOpenFiles()
{
  struct rlimit rl;
  //???不懂为什么这个为0，就是返回的已经打开的文件描述符的个数
  if (::getrlimit(RLIMIT_NOFILE, &rl)) //获取一个进程可以打开的最大文件描述符。
  {
    return openedFiles();
  }
  else
  {
    return static_cast<int>(rl.rlim_cur);//软限制的一个进程可以打开的最大文件描述符
  }
}

int ProcessInfo::numThreads()
{
  int result = 0;
  string status = procStatus();
  size_t pos = status.find("Threads:");
  if (pos != string::npos)//（pos!=-1）
  {
    result = ::atoi(status.c_str() + pos + 8);
  }
  return result;
}

std::vector<pid_t> ProcessInfo::threads()
{
  std::vector<pid_t> result;
  t_pids = &result;
  scanDir("/proc/self/task", taskDirFilter);
  t_pids = NULL;
  std::sort(result.begin(), result.end());
  return result;
}

