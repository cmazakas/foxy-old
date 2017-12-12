#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

namespace foxy
{
  auto make_acceptor(
    boost::asio::io_service&                      ios,
    boost::asio::ip::address_v4::bytes_type const bytes,
    unsigned short                          const port) 
  -> boost::asio::ip::tcp::acceptor;
}