#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h> // strerror_tl
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

// not thread safe
class LogFile::File : boost::noncopyable//File这个类主要是打开一个文件，建立文件的缓冲，然后对文件进行追加内容等操作
{
 public:
  explicit File(const string& filename)
    : fp_(::fopen(filename.data(), "ae")),//打开文件filename，其中参数a是在文件内容后面追加内容，参数e是O_CLOEXEC标志位
      writtenBytes_(0)
  {
    assert(fp_);
    ::setbuffer(fp_, buffer_, sizeof buffer_);         
    //设置fp文件的缓冲区的大小
    // posix_fadvise POSIX_FADV_DONTNEED ?
  }

  ~File()
  {
    ::fclose(fp_);
  }

  void append(const char* logline, const size_t len)//在文件末尾追加logline字符串，不带锁的写入
  {
    size_t n = write(logline, len);
    size_t remain = len - n;
	// remain>0表示没写完，需要继续写直到写完
    while (remain > 0)
    {
      size_t x = write(logline + n, remain);
      if (x == 0)
      {
        int err = ferror(fp_);
        if (err)
        {
          fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
        }
        break;
      }
      n += x;
      remain = len - n; // remain -= x
    }

    writtenBytes_ += len;
  }

  void flush()//刷新fp_的缓冲区，就是将缓冲区的内容全部写入到文件描述符中
  {
    ::fflush(fp_);
  }

  size_t writtenBytes() const { return writtenBytes_; }//返回总共向文件写入的字节数

 private:

  size_t write(const char* logline, size_t len)//fwrite_unlocked封装
  {
#undef fwrite_unlocked
    return ::fwrite_unlocked(logline, 1, len, fp_);//不带锁的写入文件
  }

  FILE* fp_;
  char buffer_[64*1024];//fp_的缓存
  size_t writtenBytes_;//一个文件总共写入的字节数
};

LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)//判断是否需要带锁，需要就以加锁的方式进行追加，否则就用不加锁的方式追加
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

void LogFile::append_unlocked(const char* logline, int len)//以不上锁的方式向文件中追加内容
{
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_)// 如果总共写的字节数大于rollSize_的值，就会重新创建一个文件
  {
    rollFile();
  }
  else
  {
    if (count_ > kCheckTimeRoll_)//如果追加次数大于1024，并且并没有在一天之中
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)//文件创建的日期不是现在的日期。就是创建文件的日期不是今天，就重新建文件
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)//如果距离上次写入文件的时间大于刷新时间间隔，就刷新一下内存
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
    else
    {
      ++count_;//每追加一次，就加1
    }
  }
}

void LogFile::rollFile()//重新创建日志
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  // 注意，这里先除kRollPerSeconds_ 后乘kRollPerSeconds_表示
  // 对齐至kRollPerSeconds_整数倍，也就是时间调整到当天零点。
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new File(filename));
  }
}

string LogFile::getLogFileName(const string& basename, time_t* now)//日志名称就是basename+.%Y%m%d-%H%M%S.+hostname+.pid+.log
//生成日志文件名字
{
  string filename;
  filename.reserve(basename.size() + 64);//预留basename.size() + 64个位置
  filename = basename;

  char timebuf[32];
  char pidbuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);//将事件按照格式化来输出字符串
  filename += timebuf;
  filename += ProcessInfo::hostname();
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;
  filename += ".log";

  return filename;
}

