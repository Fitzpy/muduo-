#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H
/* AsyncLogging就是实现一步日志的类，其中有两个主要函数threadFunc和append，append线程是由各种其他线程来执行的，就是把日志内容写到相应的
 * 缓存区中，threadFunc函数则是由一个专门线程负责，就是把缓存区中的数据写入日志文件当中*/
#include <muduo/base/BlockingQueue.h>
#include <muduo/base/BoundedBlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <muduo/base/LogStream.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

class AsyncLogging : boost::noncopyable
{
 public:

  AsyncLogging(const string& basename,
               size_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  // 
  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start(); // 
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogging(const AsyncLogging&);  // ptr_container
  void operator=(const AsyncLogging&);  // ptr_container

  // 
  void threadFunc();

  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  typedef boost::ptr_vector<Buffer> BufferVector;//FixedBuffer的数组
  typedef BufferVector::auto_type BufferPtr; // 这是一种指针类型，就是不能赋值，只能转移
                                             // 
                                             // 

  const int flushInterval_; //超时时间，在flushInterval_秒内，缓冲区没有写满，仍将缓冲区的数据写到文件中
  bool running_;//是否正在运行
  string basename_;//日志基本名称
  size_t rollSize_;//预留日志大小，日志超过这个大小就会重新建一个新的日志文件
  muduo::Thread thread_;//执行异步记录任务的线程
  muduo::CountDownLatch latch_;  // 计数器
  muduo::MutexLock mutex_;
  muduo::Condition cond_;
  BufferPtr currentBuffer_; // 当前的缓冲区
  BufferPtr nextBuffer_;    // 预备的缓冲区，muduo异步日志中采用了双缓存技术
  BufferVector buffers_;    // 缓冲区队列，每写完一个缓冲区，就把这个缓冲区挂到队列中，这些缓存都是等待待写入的文件
};

}
#endif  // MUDUO_BASE_ASYNCLOGGING_H
