#include "foxy/listener.hpp"
#include "foxy/connection.hpp"

namespace asio = boost::asio;

using boost::asio::ip::tcp;
using boost::system::error_code;

namespace foxy
{
listener::listener(
  asio::io_context&    ioc,
  tcp::endpoint const& endpoint)
: acceptor_{ioc, endpoint}
, socket_{ioc}
{
}

#include <boost/asio/yield.hpp>
auto listener::run(error_code const ec) -> void
{
  reenter (*this) {
    yield acceptor_.async_accept(
      socket_,
      [self = shared_from_this()](error_code const ec) -> void
      {
        self->run(ec);
      });

    std::make_shared<connection>(std::move(socket_))->run();
  }
}
#include <boost/asio/unyield.hpp>
} // foxy