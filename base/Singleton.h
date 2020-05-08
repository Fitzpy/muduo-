// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*单例模式，暂时的理解是，在一个进程中，只有一个这个类的实例对象，主要实现方法就是用static保证即便每次创建一个新的对象
 *其实里面的静态成员变量都是一个地址上的一个值，类似于全局变量一样。所以整个程序中只有一份类的对象
 */
#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{

template<typename T>
class Singleton : boost::noncopyable
{
 public:
  static T& instance()//初始化value，T&是返回一个引用值
  {
    pthread_once(&ponce_, &Singleton::init);//所有的线程总共只会调用一次，如果调用过，就不会调用了
    return *value_;
  }

 private:
  Singleton();
  ~Singleton();

  static void init()
  {
    value_ = new T();
    ::atexit(destroy);
    //在进程终止时，会运行atexit登记过的函数。
  }

  static void destroy()
  {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];//检测是否是完全类型
    delete value_;
  }

 private:
  static pthread_once_t ponce_;//这个变量表示pthread_once是否执行过
  static T*             value_;
};

/*静态成员函数初始化*/
template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;//初始化pthread_once函数的标志位

template<typename T>
T* Singleton<T>::value_ = NULL;

}
#endif

