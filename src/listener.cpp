#include "foxy/listener.hpp"

#include <memory>
#include <boost/system/error_code.hpp>
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
    co_await acceptor.async_accept(socket, token);

    std::make_shared<session>(std::move(socket))->start();
  }
}

} // foxy