#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <list>
#include <memory>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include "foxy/connection.hpp"

namespace foxy
{
struct listener : public std::enable_shared_from_this<listener>
{
public:
  using connection_type = connection;
  using acceptor_type   = boost::asio::ip::tcp::acceptor;
  using socket_type     = boost::asio::ip::tcp::socket;
  using coroutine_type  = boost::asio::coroutine;

private:
  std::list<connection_type> conns_;

  acceptor_type acceptor_;
  socket_type   socket_;

  coroutine_type accept_coro_;
  coroutine_type timer_coro;

public:
  listener(
    boost::asio::io_context&              io,
    boost::asio::ip::tcp::endpoint const& endpoint);

  auto accept(boost::system::error_code const ec = {}) -> void;
};

} // foxy

#endif // FOXY_LISTENER_HPP_