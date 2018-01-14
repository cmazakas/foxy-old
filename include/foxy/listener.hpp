#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <memory>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>

#include "foxy/connection.hpp"

namespace foxy
{
template <typename RouteList>
struct listener
  : public boost::asio::coroutine
  , public std::enable_shared_from_this<listener<RouteList>>
{
public:
  using acceptor_type = boost::asio::ip::tcp::acceptor;
  using socket_type   = boost::asio::ip::tcp::socket;

private:
  acceptor_type acceptor_;
  socket_type   socket_;

  RouteList const& routes_;

public:
  listener(
    boost::asio::io_context&              ioc,
    boost::asio::ip::tcp::endpoint const& endpoint,
    RouteList const& routes);

  auto run(boost::system::error_code const ec = {}) -> void;
};

template <typename RouteList>
listener<RouteList>::listener(
  boost::asio::io_context& ioc,
  boost::asio::ip::tcp::endpoint const& endpoint,
  RouteList const& routes)
: acceptor_{ioc, endpoint}
, socket_{ioc}
, routes_{routes}
{
}

#include <boost/asio/yield.hpp>
template <typename RouteList>
auto listener<RouteList>::run(boost::system::error_code const ec) -> void
{
  reenter (*this) {
    yield acceptor_.async_accept(
      socket_,
      [self = this->shared_from_this()](boost::system::error_code const ec) -> void
      {
        self->run(ec);
      });

    std::make_shared<connection<RouteList>>(std::move(socket_), routes_)->run();
  }
}
#include <boost/asio/unyield.hpp>
} // foxy


#endif // FOXY_LISTENER_HPP_