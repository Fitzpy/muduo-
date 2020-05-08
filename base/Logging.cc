#include <muduo/base/Logging.h>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Timestamp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{

/*
class LoggerImpl
{
 public:
  typedef Logger::LogLevel LogLevel;
  LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
  void finish();

  Timestamp time_;
  LogStream stream_;
  LogLevel level_;
  int line_;
  const char* fullname_;
  const char* basename_;
};
*/

__thread char t_errnobuf[512];
__thread char t_time[32];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)//根据错误号savedErrno，可以得到具体的错误代码存储在t_errnobuf数组中
{
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::LogLevel initLogLevel()//将日志等级TRACE作为返回值，返回出来
{
	return Logger::TRACE;
	/*
  if (::getenv("MUDUO_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("MUDUO_LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
	*/
}

Logger::LogLevel g_logLevel = initLogLevel();//初始化全局日志级别

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
  "TRACE ",
  "DEBUG ",
  "INFO  ",
  "WARN  ",
  "ERROR ",
  "FATAL ",
};//将日志级别对应的字符串定义出来

// helper class for known string length at compile time
class T//用一个指针存储一个字符串，再用一个长度存取字符串长度
{
 public:
  T(const char* str, unsigned len)
    :str_(str),
     len_(len)
  {
    assert(strlen(str) == len_);//确保len是字符串的长度，
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)//又重载了一次<<符号，将类型T指代的字符串加入到缓存区中
{
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)//将类型SourceFile指代的字符串加入到缓存区中
{
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char* msg, int len)
{
  size_t n = fwrite(msg, 1, len, stdout);//向标准输出，写入一个字符串msg
  //FIXME check n
  (void)n;
}

void defaultFlush()
{
  fflush(stdout);//就是把缓冲区里面的东西全部输出到设备上
}

Logger::OutputFunc g_output = defaultOutput;//定义一个函数指针是defaultOutput，默认输出函数
Logger::FlushFunc g_flush = defaultFlush;//定义一个函数指针是defaultFlush，默认更新函数

}

using namespace muduo;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
  formatTime();
  CurrentThread::tid();
  stream_ << T(CurrentThread::tidString(), 6);//将当前线程tid输出到缓冲区
  stream_ << T(LogLevelName[level], 6);//将日志等级输出到缓冲区
  if (savedErrno != 0)
  {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }//如果有错误，将错误信息输出到缓冲区
}

void Logger::Impl::formatTime()//组织时间格式，年月日 小时:分钟：秒输出到缓冲区
{
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / 1000000);
  int microseconds = static_cast<int>(microSecondsSinceEpoch % 1000000);
  if (seconds != t_lastSecond)
  {
    t_lastSecond = seconds;
    struct tm tm_time;
    ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
    //tm结构计算年就是从1900年开始算起
    int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17); (void)len;
  }
  Fmt us(".%06dZ ", microseconds);
  assert(us.length() == 9);
  stream_ << T(t_time, 17) << T(us.data(), 9);//把组织好的事件格式以及剩余的微秒时间输出到缓冲区中
}

void Logger::Impl::finish()
{
  stream_ << " - " << basename_ << ':' << line_ << '\n';//向缓冲区中输入结束符
}

//一系列Logger构造函数，就是将锁提供的信息输入到缓冲区中
Logger::Logger(SourceFile file, int line)
  : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)//错误信息的构造函数
  : impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());//得到一份缓冲区类FixedBuffer的拷贝，然后用缓冲区的内容来执行输出函数
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL)
  {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(Logger::LogLevel level)//设置日志等级
{
  g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)//设置输出函数
{
  g_output = out;
}

void Logger::setFlush(FlushFunc flush)//设置刷新函数
{
  g_flush = flush;
}
