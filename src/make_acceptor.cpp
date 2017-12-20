#include "foxy/make_acceptor.hpp"

namespace io = boost::asio;
namespace ip = boost::asio::ip;

using io::io_service;

namespace foxy
{
  auto make_acceptor(
    io_service&                      ios,
    ip::address_v4::bytes_type const bytes,
    unsigned short             const port) -> ip::tcp::acceptor
  {
    return ip::tcp::acceptor{
      ios,
      ip::tcp::endpoint{
        ip::make_address_v4(bytes),
        port}
    };
  }
}