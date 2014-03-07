#include <iostream>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main(int argc, char** argv)
{
  try {
    boost::asio::io_service ioService;
    tcp::acceptor acceptor(ioService, tcp::endpoint(tcp::v4(), 2678));

    for(;;) {
      tcp::socket socket(ioService);
      acceptor.accept(socket);

      std::string msg = "Connected to AgFS server\n";
      
      boost::system::error_code ie;
      boost::asio::write(socket, boost::asio::buffer(msg), ie);
    }
  } catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
