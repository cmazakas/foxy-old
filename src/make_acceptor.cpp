#include "foxy/make_acceptor.hpp"
#include "foxy/make_v4_endpoint.hpp"

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
    return ip::tcp::acceptor{ios, make_v4_endpoint(bytes, port)};
  }
}