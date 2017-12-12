#include "foxy/make_v4_endpoint.hpp"

namespace ip = boost::asio::ip;

namespace foxy
{
  auto make_v4_endpoint(
    ip::address_v4::bytes_type const bytes,
    unsigned short             const port) -> ip::tcp::endpoint
  {
    auto const address = ip::address_v4{bytes};
    return {address, port};
  }
}