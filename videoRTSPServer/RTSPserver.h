#pragma once
#include <boost/asio/io_context.hpp>
#include <cstdint>
#include <memory>

#include "global.h"
#include "threadpool.h"
class RTSPServer : public std::enable_shared_from_this<RTSPServer> {
 public:
  RTSPServer(net::io_context& ioc, uint16_t port);
  ~RTSPServer();
  void start();

 private:
  void newConnection();

 private:
  net::io_context& ioc_;
  tcp::acceptor acceptor_;
  std::shared_ptr<ThreadPool> threadpool_ = ThreadPool::GetInstance();
};
