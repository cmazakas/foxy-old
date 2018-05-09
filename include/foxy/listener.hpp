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
  boost::asio::io_context&              io,
  boost::asio::ip::tcp::endpoint const& endpoint,
  Routes const&                         routes
) -> awaitable<void>
{
  using boost::asio::ip::tcp;
  using boost::system::error_code;

  std::cout << "beginning execution...\n";

  auto ec          = error_code();
  auto token       = co_await this_coro::token();
  auto error_token = make_redirect_error_token(token, ec);

  std::cout << "building socket and acceptor...\n";

  auto socket   = tcp::socket(io);

  std::cout << "built socket...\n";

  try {
    auto acceptor = tcp::acceptor(io, endpoint);

    std::cout << "listening now...\n";

    co_await acceptor.async_accept(socket, error_token);
    if (ec) {
      fail(ec, "accept");
    }

    std::cout << "accepted connection...\n";

    std::make_shared<session<Routes>>(std::move(socket), routes)->start();

  } catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    throw e ;
  }




  // for (;;) {
  //   boost::ignore_unused(co_await acceptor.async_accept(socket, token));
  //   if (ec) {
  //     fail(ec, "accept");
  //     continue;
  //   }

  //   std::make_shared<session<Routes>>(std::move(socket), routes)->start();
  // }
}

} // foxy

#endif // FOXY_LISTENER_HPP_