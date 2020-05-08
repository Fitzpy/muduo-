// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Condition.h>

#include <errno.h>

// returns true if time out, false otherwise.
bool muduo::Condition::waitForSeconds(int seconds)
{
  struct timespec abstime;
  clock_gettime(CLOCK_REALTIME, &abstime);//获得时间，从1970-1-1 0:0:0开始计时
  abstime.tv_sec += seconds;//希望等待的时间，也就是到
  return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
  //在等待过程中是可以触发条件的。
}

