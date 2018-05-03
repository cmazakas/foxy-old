#include "foxy/listener.hpp"

#include <memory>
#include <boost/system/error_code.hpp>
#include <boost/core/ignore_unused.hpp>

#include "foxy/log.hpp"
#include "foxy/session.hpp"

using boost::asio::ip::tcp;
using boost::system::error_code;

namespace foxy
{

auto listener(
  boost::asio::io_context&              io,
  boost::asio::ip::tcp::endpoint const& endpoint
) -> awaitable<void>
{
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

    std::make_shared<session>(std::move(socket))->start();
  }
}

} // foxy