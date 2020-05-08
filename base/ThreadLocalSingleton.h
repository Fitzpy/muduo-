// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*既是线程特定数据，又是单例模式。也就是这个类只能有一个类变量，并且这个类变量可以在每个线程中单独修改，而不互相影响
 *具体来说就是t_value_通过关键词__thread来保证线程局部性，然后t_value_指针指向的数据通过线程的特定数据特性来保证线程局部性
 *其他的变量在程序中只有一份。
 */
#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocalSingleton : boost::noncopyable
{
 public:

  static T& instance()//初始化t_value_
  {
    if (!t_value_)
    {
      t_value_ = new T();
      deleter_.set(t_value_);
    }
    return *t_value_;
  }

  static T* pointer()//返回t_value_的值
  {
    return t_value_;
  }

 private:

  static void destructor(void* obj)//销毁t_value_
  {
    assert(obj == t_value_);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    delete t_value_;
    t_value_ = 0;
  }

  /*这是ThreadLocalSingleton类中的一个类，这个类主要是操作线程的特定数据的，和ThreadLocal类有点像*/
  class Deleter
  {
   public:
    Deleter()
    {
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
    }

    ~Deleter()
    {
      pthread_key_delete(pkey_);
    }

    void set(T* newObj)//设置每个线程自己的数据块
    {
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;
  };

  static __thread T* t_value_;
  static Deleter deleter_;
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}
#endif
