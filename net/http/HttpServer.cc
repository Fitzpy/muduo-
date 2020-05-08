// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <muduo/net/http/HttpServer.h>

#include <muduo/base/Logging.h>
#include <muduo/net/http/HttpContext.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

namespace muduo
{
namespace net
{
namespace detail
{

// FIXME: move to HttpContext class
//解析请求行的报头，主要获得请求方法，请求URL和请求方法1.1还是1.0
//一般请求行相应格式如下：
//GET http://localhost:8000/home.html HTTP/1.1
bool processRequestLine(const char* begin, const char* end, HttpContext* context)//begin和end指向请求信息的头部和尾部
{
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');//找到第一个空格处
  HttpRequest& request = context->request();//把解析出来的内容填入context->request_中
  if (space != end && request.setMethod(start, space))		// 解析请求方法
  {
    start = space+1;
    space = std::find(start, end, ' ');//找到第二个空格处
    if (space != end)
    {
      request.setPath(start, space);	// 解析PATH
      start = space+1;
      succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
      if (succeed)
      {
        if (*(end-1) == '1')
        {
          request.setVersion(HttpRequest::kHttp11);		// HTTP/1.1
        }
        else if (*(end-1) == '0')
        {
          request.setVersion(HttpRequest::kHttp10);		// HTTP/1.0
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// FIXME: move to HttpContext class
// return false if any error
//解析http请求包
//http数据包有以下几个部分组成
//请求行
//请求报头
//请求体（一般是post方法才会有，这个server不支持有body的）
bool parseRequest(Buffer* buf, HttpContext* context, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (context->expectRequestLine())	// 处于解析请求行状态
    {
      const char* crlf = buf->findCRLF();//找到终止符
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf, context);	// 解析请求行
        if (ok)
        {
          context->request().setReceiveTime(receiveTime);		// 设置请求时间
          buf->retrieveUntil(crlf + 2);		// 将请求行从buf中取出，包括\r\n
          context->receiveRequestLine();	// 将HttpContext状态改为kExpectHeaders
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (context->expectHeaders())		// 解析请求报头，请求报头的格式“报头名称：报头数据”
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');		//冒号所在位置
        if (colon != crlf)//如果相等，那么就说明是空行，报头解析就结束了
        {
          context->request().addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          context->receiveHeaders();		// HttpContext将状态改为kGotAll
          hasMore = !context->gotAll();
        }
        buf->retrieveUntil(crlf + 2);		// 将header从buf中取回，包括\r\n
      }
      else
      {
        hasMore = false;
      }
    }
    else if (context->expectBody())			// 当前还不支持请求中带body
    {
      // FIXME:
    }
  }
  return ok;
}

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

}
}
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name)
  : server_(loop, listenAddr, name),
    httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(
      boost::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      boost::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

HttpServer::~HttpServer()
{
}

void HttpServer::start()
{
  LOG_WARN << "HttpServer[" << server_.name()
    << "] starts listenning on " << server_.hostport();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)//这个函数绑定到TcpServer::connectionCallback_上，
//其实就是绑定到TcpConnection::connectionCallback_上，也就是在和客户端建立连接以后，以及断开连接前，会调用这个函数
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());	// TcpConnection与一个HttpContext绑定
  }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)//这个函数绑定在TcpConnection::messageCallback_上，会在TCpConnection的channel读函数中调用
{
  HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

  if (!detail::parseRequest(buf, context, receiveTime))
  {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  // 请求消息解析完毕
  if (context->gotAll())
  {
    onRequest(conn, context->request());
    context->reset();		// 本次请求处理完毕，重置HttpContext，适用于长连接
  }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
  const string& connection = req.getHeader("Connection");
  bool close = connection == "close" ||
    (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}

