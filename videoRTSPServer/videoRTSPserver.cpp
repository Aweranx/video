#include <iostream>
#include <memory>

#include "RTSPserver.h"
auto main() -> int {
  try {
    net::io_context ioc{1};
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](const boost::system::error_code& error, int signal_number) {
          if (error) {
            return;
          }
          ioc.stop();
        });
    std::make_shared<RTSPServer>(ioc, 8554)->start();
    ioc.run();
  } catch (std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}