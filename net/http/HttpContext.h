// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.
/*存储状态机的类，并且有一个HttpRequest作为成员变量，HttpRequestParseState存储的是解析这个HttpRequest到哪一个状态了*/
#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include <muduo/base/copyable.h>

#include <muduo/net/http/HttpRequest.h>

namespace muduo
{
namespace net
{

class HttpContext : public muduo::copyable
{
 public:
  enum HttpRequestParseState//解析请求状态的枚举
  {
    kExpectRequestLine,//当前正处于解析请求行的步骤
    kExpectHeaders,//当前正处于解析请求头部的步骤
    kExpectBody,//当前正处于解析请求实体的步骤
    kGotAll,//解析完毕状态
  };

  HttpContext()
    : state_(kExpectRequestLine)//初始状态，期望收到请求行
  {
  }

  // default copy-ctor, dtor and assignment are fine

  bool expectRequestLine() const
  { return state_ == kExpectRequestLine; }

  bool expectHeaders() const
  { return state_ == kExpectHeaders; }

  bool expectBody() const
  { return state_ == kExpectBody; }

  bool gotAll() const
  { return state_ == kGotAll; }

  void receiveRequestLine()
  { state_ = kExpectHeaders; }

  void receiveHeaders()
  { state_ = kGotAll; }  // FIXME

  // 重置HttpContext状态
  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& request() const
  { return request_; }

  HttpRequest& request()
  { return request_; }

 private:
  HttpRequestParseState state_;		// 请求解析状态
  HttpRequest request_;				// http请求
};

}
}

#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H
