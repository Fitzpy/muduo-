// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/ThreadPool.h>

#include <muduo/base/Exception.h>

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& name)
  : mutex_(),
    cond_(mutex_),
    name_(name),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

//开启线程池，并开启numThreads个线程
void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());//如果智能指针数组为空，也就是线程池中没有线程，就会停止
  running_ = true;
  threads_.reserve(numThreads);//预留了numThreads个空间
  //这个循环主要是创建numThreads个线程，并且每个线程类Thread的ThreadFunc函数都是ThreadPool::runInThread
  //这个函数就是每个线程在没有任务时一直条件阻塞等待，如果有任务了，先取任务队列前面的去执行
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i);//就是将后面两个参数格式化以后的字符串拷贝到id字符串指针中，并且拷贝的最大长度是sizeof（id）
    threads_.push_back(new muduo::Thread(
          boost::bind(&ThreadPool::runInThread, this), name_+id));
    //Thread类的初始化第一个参数是一个boost::function智能函数指针，所以这里的函数指针参数就是被绑定的runInThread函数
    //当boost::bind绑定类内部成员时，第二个参数必须是类的实例，这里用的是this
    threads_[i].start();//创建线程
  }
}

//关闭线程池
void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  running_ = false;
  cond_.notifyAll();//将running_改为false并且广播了所有线程以后，线程就不再阻塞了，会一直抢锁运行，这样就可以为下一步释放做准备
  }
  for_each(threads_.begin(),
           threads_.end(),
           boost::bind(&muduo::Thread::join, _1));//逐一销毁线程，
           //？？？但是不理解为什么join函数没有入口参数，但是这里却有一个站位符，并且根据for_each内容理解，
           //*threads_.begin()就是join的入口参数，没看懂
}

//将认为存入任务队列中
void ThreadPool::run(const Task& task)
{
  if (threads_.empty())//如果线程池没有线程，就直接执行
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(task);//存入任务队列中
    cond_.notify();//发出条件信号，让线程来领取任务。
  }
}

//take函数是每个线程都执行的，需要考虑线程安全，考虑多线程下取任务的安全性，所以要加锁加条件变量
ThreadPool::Task ThreadPool::take()//取任务函数
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_)//如果任务队列是空的，并且线程池一直在运行，就一直在条件等待
  //之所以这边加了一个循环，是因为假设这样一种情况，假设之前没有一个任务，线程池中所有线程都在等待条件信号，这时候来了一个信号
  //会产生惊群效应， cond_.notify()会把所有的线程唤醒，但是只有一个线程可以抢到锁，所以其他的线程就要在while循环下继续阻塞等待
  {
    cond_.wait();
  }
  Task task;
  if(!queue_.empty())//如果任务队列不为空
  {
    task = queue_.front();//把队列中第一个任务拿出来
    queue_.pop_front();//把队列中第一个任务从队列中拿出去
  }
  return task;//返回的就是队列中第一个任务
}

void ThreadPool::runInThread()//运行线程任务
{
  try
  {
    while (running_)
    {
      Task task(take());//定义一个Task变量，然后运行take函数，将这个函数的返回值作为变量初始化的内容
      if (task)
      {
        task();
      }
    }
  }
  //异常处理
  catch (const Exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

