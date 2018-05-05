#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include <boost/core/ignore_unused.hpp>

#include "foxy/log.hpp"
#include "foxy/session.hpp"
#include "foxy/coroutine.hpp"

namespace foxy
{

template <typename Routes>
auto listener(
  boost::asio::io_context&              io,
  boost::asio::ip::tcp::endpoint const& endpoint,
  Routes const&                         routes
) -> awaitable<void>
{
  using boost::asio::ip::tcp;
  using boost::system::error_code;

  auto ec       = error_code();
  auto token    = make_redirect_error_token(co_await this_coro::token(), ec);
  auto socket   = tcp::socket(io);
  auto acceptor = tcp::acceptor(io, endpoint);

  for (;;) {
    boost::ignore_unused(co_await acceptor.async_accept(socket, token));
    if (ec) {
      fail(ec, "accept");
      continue;
    }

    std::make_shared<session<Routes>>(std::move(socket), routes)->start();
  }
}

} // foxy

#endif // FOXY_LISTENER_HPP_