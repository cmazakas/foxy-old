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

struct session : public std::enable_shared_from_this<session>
{
public:
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = boost::asio::ip::tcp::socket;
  using parser_type = header_parser<>;
  using strand_type = boost::asio::strand<
    boost::asio::io_context::executor_type>;

  using timer_type = boost::asio::steady_timer;

private:
  stream_type stream_;
  strand_type strand_;
  timer_type  timer_;
  buffer_type buffer_;
  parser_type parser_;

public:

  session(stream_type stream);

  template <typename Routes>
  auto start(Routes const& routes) -> void
  {
    co_spawn(
      strand_,
      [self = this->shared_from_this(), routes]()
      {
        return self->request_handler<decltype(routes)>(routes);
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

  template <typename Routes>
  auto request_handler(Routes const& routes) -> awaitable<void, strand_type>
  {
    namespace http  = boost::beast::http;
    namespace beast = boost::beast;

    auto self = this->shared_from_this();

    auto ec          = boost::system::error_code();
    auto token       = co_await this_coro::token();
    auto executor = co_await this_coro::executor();

    boost::ignore_unused(co_await http::async_read_header(
      stream_,
      buffer_, parser_,
      token));

    match_route(
      parser_.get().target(),
      routes,
      strand_,
      ec, self);
  }

  auto timeout() -> awaitable<void, strand_type>;

  auto buffer() & noexcept -> buffer_type&;
  auto stream() & noexcept -> stream_type&;
  auto parser() & noexcept -> parser_type&;
  auto timer()  & noexcept -> timer_type&;
};


} // foxy

#endif // FOXY_SESSION_HPP_