// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Exception.h>
//#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{
namespace CurrentThread
{
  // __thread修饰的变量是线程局部存储的，就是每个线程有自己单独的一份变量。
  __thread int t_cachedTid = 0;		// 线程真实pid（tid）的备份，
									// 是为了减少::syscall(SYS_gettid)系统调用的次数
									// 提高获取tid的效率
  __thread char t_tidString[32];	// 这是tid的字符串表示形式
  __thread const char* t_threadName = "unknown";
  const bool sameType = boost::is_same<int, pid_t>::value;//判断int和pid_t是否一致,并把这个value传给sameType
  BOOST_STATIC_ASSERT(sameType);//BOOST_STATIC_ASSERT效果类似于assert，如果参数为0，会发出提示，并且终止程序，并且在
  //编译时也会这样，这就是比assert优势的地方
}

namespace detail
{
//系统调用获得当前线程ID
pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

//创建子进程以后，在子进程中重置t_cachedTid的值
void afterFork()
{
  muduo::CurrentThread::t_cachedTid = 0;//清零tid，为了执行CurrentThread::tid()中的cacheTid函数
  muduo::CurrentThread::t_threadName = "main";
  CurrentThread::tid();//重置t_cachedTid的值
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    pthread_atfork(NULL, NULL, &afterFork);//pthread_atfork中三个函数分别是在进程创建之前的父进程中，在进程创建之后的父进程中
    //在进程创建之后的子进程中的调用的函数指针
  }
};

ThreadNameInitializer init;//定义一个init变量，先于main函数就初始化了
}
}

using namespace muduo;
/*将当前线程id缓存起来*/
void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    assert(n == 6); (void) n;
  }
}

//判断是否是主线程
bool CurrentThread::isMainThread()
{
  return tid() == ::getpid();
}

AtomicInt32 Thread::numCreated_;

//线程类的构造函数
Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n)
{
  numCreated_.increment();//每创建一个函数，原子类就加1
}

Thread::~Thread()
{
  // no join
}

//创建一个子线程
void Thread::start()
{
  assert(!started_);//判断初始化是否成功
  started_ = true;
  errno = pthread_create(&pthreadId_, NULL, &startThread, this);
  if (errno != 0)
  {
    //LOG_SYSFATAL << "Failed in pthread_create";
  }
}

//(？？？为什么join一直没有在其他类成员函数中使用，一般不都是创建线程以后就使用的吗？可能是线程是一直运行的，像线程池那样？)
//解答：这里对join和detach的理解有一些偏差，join是在主线程中调用，结束子线程的。
//在线程池结束的时候，会一一释放这些函数
int Thread::join()
{
  assert(started_);
  return pthread_join(pthreadId_, NULL);
}

void* Thread::startThread(void* obj)
{
  Thread* thread = static_cast<Thread*>(obj);
  thread->runInThread();
  return NULL;
}

void Thread::runInThread()
{
  tid_ = CurrentThread::tid();
  muduo::CurrentThread::t_threadName = name_.c_str();//c_str将string转换成c_str
  try
  {
    func_();
    muduo::CurrentThread::t_threadName = "finished";
  }
  catch (const Exception& ex)//???上下两个有什么不一样
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    muduo::CurrentThread::t_threadName = "crashed";
    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
    throw; // rethrow
  }
}

