#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include <memory>
#include <utility>

#include "foxy/connection.hpp"

namespace foxy
{
template <typename RouteList>
struct listener
  : public boost::asio::coroutine
  , public std::enable_shared_from_this<listener<RouteList>>
{
private:
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket   socket_;

  boost::fusion::list<RouteList> routes_;

public:
  listener(
    boost::asio::io_context&              ioc,
    boost::asio::ip::tcp::endpoint const& endpoint,

    RouteList const  routes);

  auto run(boost::system::error_code const ec = {}) -> void;
};

template <typename RouteList>
listener<RouteList>::listener(
  boost::asio::io_context&              ioc,
  boost::asio::ip::tcp::endpoint const& endpoint,
  RouteList const  routes
)
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
      [self = shared_from_this()](boost::system::error_code const ec) -> void
      {
        self->run(ec);
      });

    std::make_shared<connection>(std::move(socket_))->run();
  }
}
#include <boost/asio/unyield.hpp>
} // foxy


#endif // FOXY_LISTENER_HPP_