// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
/*原子操作函数，
 *这里面只有__sync_val_compare_and_swap这样的函数才是真的原子函数
 *,也就是这种函数在执行过程中 ，不会被其他线程打断
 *,所以进一步封装以后，AtomicIntegerT类中的成员函数其实不是严格意义上的原子函数，但是由于最关键的对变量值的改写操作，已经通过
 *原子函数操作完成了，所以后面的return等操作就无关紧要了，所以可以把整个类看成是原子操作*/
#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace muduo
{

namespace detail
{
template<typename T>
class AtomicIntegerT : boost::noncopyable
{
 public:
  AtomicIntegerT()
    : value_(0)
  {
  }

  // uncomment if you need copying and assignment
  //
  // AtomicIntegerT(const AtomicIntegerT& that)
  //   : value_(that.get())
  // {}
  //
  // AtomicIntegerT& operator=(const AtomicIntegerT& that)
  // {
  //   getAndSet(that.get());
  //   return *this;
  // }

  T get()//得到value的值
  {
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T getAndAdd(T x)//先获取value的值，再执行value+x，value=oldvalue+x
  {
    return __sync_fetch_and_add(&value_, x);
  }

  T addAndGet(T x)//先加x，在获取，类中的value值也加上了x
  {
    return getAndAdd(x) + x;
    /*getAndAdd返回值是value，并且此时value=oldvalue+x
     *addAndGet返回值也是oldvalue+x
    */
  }

  T incrementAndGet()//value+1,再获取
  {
    return addAndGet(1);
  }

  T decrementAndGet()//value-1，再获取
  {
    return addAndGet(-1);
  }

  void add(T x)//value+x，无返回值
  {
    getAndAdd(x);
  }

  void increment()//value+1.无返回值
  {
    incrementAndGet();
  }

  void decrement()//value-1,无返回值
  {
    decrementAndGet();
  }

  T getAndSet(T newValue)//先得到value值，再设置为newValue新值
  {
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;//volatile关键词的意思是，每次读取value_值时，都是从该地址，而不是暂存的寄存器中读取
};
}

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;
}

#endif  // MUDUO_BASE_ATOMIC_H
