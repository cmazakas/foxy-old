#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <memory>
#include <iostream>
#include <exception>

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
  boost::asio::io_context&       io,
  boost::asio::ip::tcp::endpoint endpoint,
  Routes const&                  routes
) -> awaitable<void>
{
  using boost::asio::ip::tcp;
  using boost::system::error_code;

  auto ec          = error_code();
  auto token       = co_await this_coro::token();
  auto error_token = make_redirect_error_token(token, ec);

  auto socket   = tcp::socket(io);
  auto acceptor = tcp::acceptor(io, endpoint);

  for (;;) {
    boost::ignore_unused(co_await acceptor.async_accept(socket, error_token));
    if (ec) {
      fail(ec, "accept");
      continue;
    }

    std::make_shared<session>(
      std::move(socket))->start<decltype(routes)>(routes);
  }
}

} // foxy

#endif // FOXY_LISTENER_HPP_