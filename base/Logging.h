#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include <muduo/base/LogStream.h>
#include <muduo/base/Timestamp.h>
/* Logger类是一个生命周期非常短的类，在使用的过程中基本就是使用类的临时变量，在类创建时将日志信息输出到buffer区，然后在类析构时将Buffer区
 * 的内容调用输出函数去运行，所以一般都是使用宏定义来创建类的临时变量
 * */
namespace muduo
{

class Logger
{
 public:
  enum LogLevel//log级别
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile//这个类只有两个函数，两个重载的构造函数，这两个函数都是做同一件事，就是找到输入参数中‘/’符号最后一次出现的位置，然后根据这个符号把字符串截断
  //其实就是用来找到源文件名字
  {
   public:
    template<int N>
    inline SourceFile(const char (&arr)[N])
      : data_(arr),
        size_(N-1)
    {
      const char* slash = strrchr(data_, '/'); // builtin function，查找到/符号最后一次出现的位置，并且将该符号之后的位置给data_指针
      if (slash)
      {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename)
      : data_(filename)
    {
      const char* slash = strrchr(filename, '/');
      if (slash)
      {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;
    int size_;
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);//输出函数
  typedef void (*FlushFunc)();//更新函数
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

 private:

class Impl//logger类内部的一个嵌套类，封装了Logger的缓冲区LogStream
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
  void formatTime();
  void finish();

  Timestamp time_;//当前的时间
  LogStream stream_;//日志缓冲区，及通过流的方式，也就是使用<<符号，对缓冲区进行操作
  LogLevel level_;//日志等级
  int line_;//logger类所在行数，一般为__LINE__
  SourceFile basename_;//logger类源文件名称，一般为__FILE__
};

  Impl impl_;

};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()//返回全局的日志等级
{
  return g_logLevel;
}

//如果全局变量日志等级为TRACE，就定义LOG_TRACE宏，也就是创建一个临时变量来使用，可以实现LOG_TRACE<<"要存入的字符串"这样一个形式。
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
//如果全局变量日志等级为TRACE和DEBUG
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
//如果全局变量日志等级为TRACE和DEBUG或者INFO
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr) {
  if (ptr == NULL) {
   Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}

#endif  // MUDUO_BASE_LOGGING_H
