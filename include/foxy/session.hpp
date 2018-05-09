#ifndef FOXY_SESSION_HPP_
#define FOXY_SESSION_HPP_

#include <memory>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/core/ignore_unused.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/system/error_code.hpp>

#include "foxy/log.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/match_route.hpp"
#include "foxy/header_parser.hpp"

namespace foxy
{

template <typename Routes>
struct session : public std::enable_shared_from_this<session<Routes>>
{
public:
  using socket_type = boost::asio::ip::tcp::socket;
  using strand_type = boost::asio::strand<
    boost::asio::io_context::executor_type>;

private:
  socket_type   socket_;
  strand_type   strand_;
  Routes const& routes_;

public:
  explicit
  session(socket_type socket, Routes const& routes)
  : socket_(std::move(socket))
  , strand_(socket.get_executor())
  , routes_(routes)
  {
  }

  explicit
  session(socket_type socket, Routes const&& routes) = delete;

  auto start(void) -> void
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

  auto request_handler() -> awaitable<void, strand_type>
  {
    namespace http  = boost::beast::http;
    namespace beast = boost::beast;

    auto ec          = boost::system::error_code();
    auto token       = co_await this_coro::token();
    auto error_token = make_redirect_error_token(token, ec);

    auto executor = co_await this_coro::executor();

    header_parser<> parser;

    auto buffer = beast::flat_buffer();

    boost::ignore_unused(
      co_await http::async_read_header(
        socket_,
        buffer, parser,
        error_token));

    if (ec) {
      co_return fail(ec, "read header");
    }

    match_route(
      parser.get().target(),
      routes_,
      executor,
      ec, socket_, parser);
  }

  auto timeout(void) -> awaitable<void, strand_type>
  {
    co_return;
  }
};


} // foxy

#endif // FOXY_SESSION_HPP_