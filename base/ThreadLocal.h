// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*线程特定数据操作类，包括创建线程的特定数据，得到特定数据的指针，释放特定数据等，特定数据的类型为T，自定义的
 *线程特定数据是指每个线程有自己单独的数据，而不是公用一个数据，这样就可以保证线程安全
 */
#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocal : boost::noncopyable
{
 public:
  ThreadLocal()
  {
    pthread_key_create(&pkey_, &ThreadLocal::destructor);
  }

  ~ThreadLocal()
  {
    pthread_key_delete(pkey_);
  }

  T& value()//得到指向特定数据的指针
  {
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));//得到元素下标pkey_对应的数据地址指针
    if (!perThreadValue) {//只有当pkey_对应的数据地址指针为空时，才会新建一块内存地址，将指针赋值给pkey_
      T* newObj = new T();
      pthread_setspecific(pkey_, newObj);//设置数据地址指针指向的值
      perThreadValue = newObj;
    }
    return *perThreadValue;
  }

 private:
  //销毁函数，当线程终止或者结束时，释放动态内存
  static void destructor(void *x)
  {
    T* obj = static_cast<T*>(x);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];//检测是否是完全类型
    delete obj;
  }

 private:
  pthread_key_t pkey_;//就是Key结构体数组的元素下标
};

}
#endif
