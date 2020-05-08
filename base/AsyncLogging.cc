#include <muduo/base/AsyncLogging.h>
#include <muduo/base/LogFile.h>
#include <muduo/base/Timestamp.h>

#include <stdio.h>

using namespace muduo;

AsyncLogging::AsyncLogging(const string& basename,
                           size_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(boost::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len)//将logline字符串加入进缓存中
{
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)//如果当前缓存足够，就直接加入进当前缓存
  {
    // 
    currentBuffer_->append(logline, len);
  }
  else//如果第一块缓存不够
  {
    // 
    buffers_.push_back(currentBuffer_.release());//将currentBuffer_指针指向的内容转移到buffers_数组中

    // 
    if (nextBuffer_)//把下一块缓存置换到当前缓存来
    {
      currentBuffer_ = boost::ptr_container::move(nextBuffer_); // 
    }
    else//下一块缓存是空的就新建一块缓存
    {
      // 
	  // 
      currentBuffer_.reset(new Buffer); // Rarely happens
    }
    currentBuffer_->append(logline, len);//追加logline字符串
    cond_.notify(); //通知线程去处理，只有在第一块缓存不够的情况下，才会通知后端处理线程去处理，相当于攒一块缓存的日志，然后用IO写入文件
  }
}

void AsyncLogging::threadFunc()//处理线程的函数
{
  assert(running_ == true);
  latch_.countDown();
  LogFile output(basename_, rollSize_, false);
  //
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      muduo::MutexLockGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        cond_.waitForSeconds(flushInterval_); // 如果buffers_缓冲队列中没有需要处理的缓存，就条件阻塞flushInterval_时间，
        //期间如果有线程将写满的缓存块挂在buffers_缓冲队列中，就会唤醒，否则就是到时间自动唤醒
      }
      buffers_.push_back(currentBuffer_.release()); // 把当前正在使用的缓存块放入buffers_缓冲队列中
      currentBuffer_ = boost::ptr_container::move(newBuffer1); // 给当前缓冲块置新值，
      //注意这里是将newBuffer1转换成右值，然后再赋给currentBuffer_,相当于把newBuffer1的值移动给了currentBuffer_
      //在这之后，newBuffer1就为空了，如果使用，就会报出内存溢出
      buffersToWrite.swap(buffers_); // 将buffers_缓冲队列置换过来处理，并交换一个空的buffers_缓冲队列
      if (!nextBuffer_)
      {
        nextBuffer_ = boost::ptr_container::move(newBuffer2); // 将预备缓存也换成新的
                                                              // 
      }
    }//在上锁这段时间中，是不会产生新的缓存的
    //由于IO操作比较浪费时间，所以下面这一块很可能需要执行很长时间，这时在前端如果有很多线程在运行，并且不断向缓存中写入数据，所以
    //等到下一次再来处理缓存时，会有比较大的缓存需要处理。
    assert(!buffersToWrite.empty());

    // 
    if (buffersToWrite.size() > 25)//如果在buffers_缓冲队列中的缓存块个数大于25，就把前三个缓存块保存下来，其他都去掉
    {
      char buf[256];//保存的是log缓存太多，丢弃了许多log缓存
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);
      fputs(buf, stderr);//把buf字符串写入到stderr标准错误流中
      output.append(buf, static_cast<int>(strlen(buf)));//把buf字符串追加到日志文件中
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end()); // 只保留前三个缓存快
    }
    for (size_t i = 0; i < buffersToWrite.size(); ++i)
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffersToWrite[i].data(), buffersToWrite[i].length());//将缓存块队列中的缓存块内容写入文件当中
    }

    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2); // 
    }
    //如果buffersToWrite的尺寸小于2，那就说明nextBuffer_是空的，就不会进入if (!newBuffer2)判断

    if (!newBuffer1)//newBuffer1肯定为空
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.pop_back();//由于之前把newBuffer1的值移动给了currentBuffer_，所以如果再次调用就会出错，
      //所以需要使用给newBuffer1赋另外一个值，然后把缓存中的数据清空
      newBuffer1->reset();
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}

