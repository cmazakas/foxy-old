#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address_v4.hpp>

namespace foxy
{
  auto make_v4_endpoint(
    boost::asio::ip::address_v4::bytes_type const bytes,
    unsigned short                          const port) 
  -> boost::asio::ip::tcp::endpoint;
}