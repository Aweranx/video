#include "RTSPserver.h"

#include <boost/system/detail/error_code.hpp>
#include <iostream>
#include <memory>

#include "asioioservicepool.h"
#include "RTSPsession.h"

RTSPServer::RTSPServer(net::io_context& ioc, uint16_t port)
    : ioc_(ioc), acceptor_(ioc_, tcp::endpoint(tcp::v4(), port)) {}

RTSPServer::~RTSPServer() {}

void RTSPServer::start() {
  
  auto self = shared_from_this();
  auto& ioc = AsioIOServicePool::GetInstance()->GetIOService();
  auto new_session = std::make_shared<RTSPSession>(ioc);
  acceptor_.async_accept(
      new_session->Socket(), [self, new_session](boost::system::error_code ec) {
        try {
          // 出错则放弃这个连接，继续监听新链接
          if (ec) {
            self->start();
            return;
          }
          // 处理新链接，创建session管理新连接
          std::cout << "新连接" << std::endl;
          new_session->pickRequest();
          // 继续监听
          self->start();
        } catch (std::exception& exp) {
          std::cout << "exception is " << exp.what() << std::endl;
          self->start();
        }
      });
}