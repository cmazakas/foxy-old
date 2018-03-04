#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_


#include <memory>

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
  using acceptor_type   = boost::asio::ip::tcp::acceptor;
  using socket_type     = boost::asio::ip::tcp::socket;
  using coroutine_type  = boost::asio::coroutine;

private:
  acceptor_type acceptor_;
  socket_type   socket_;

  coroutine_type accept_coro_;

public:
  listener(
    boost::asio::io_context&              io,
    boost::asio::ip::tcp::endpoint const& endpoint);

  auto accept(boost::system::error_code const ec = {}) -> void;
};

} // foxy

#endif // FOXY_LISTENER_HPP_