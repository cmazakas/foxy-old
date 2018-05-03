#include "foxy/session.hpp"

#include <boost/core/ignore_unused.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/system/error_code.hpp>

#include "foxy/log.hpp"
#include "foxy/header_parser.hpp"

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

namespace foxy
{

session::session(socket_type socket)
: socket_(std::move(socket))
, strand_(socket.get_executor())
{
}

auto session::start(void) -> void
{
  co_spawn(
    strand_,
    [self = this->shared_from_this()]()
    {
      return self->request_handler();
    },
    detached);

  co_spawn(
    strand_,
    [self = this->shared_from_this()]()
    {
      return self->timeout();
    },
    detached);
}

auto session::request_handler(void) -> awaitable<void, strand_type>
{
  auto ec    = boost::system::error_code();
  auto token = make_redirect_error_token(co_await this_coro::token(), ec);

  header_parser<> parser;

  auto buffer = beast::flat_buffer();

  boost::ignore_unused(
    co_await http::async_read_header(socket_, buffer, parser, token));
  if (ec) {
    co_return fail(ec, "read header");
  }

  co_return;
}

auto session::timeout(void) -> awaitable<void, strand_type>
{
  co_return;
}

} // foxy