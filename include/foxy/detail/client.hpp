#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/type_traits.hpp>

#include <boost/system/error_code.hpp>

#include <boost/core/ignore_unused.hpp>

#include "foxy/coroutine.hpp"

namespace foxy
{
namespace proto
{
namespace detail
{

template <
  typename AsyncStream,
  typename RequestSerializer,
  typename ResponseParser,
  typename Buffer
>
auto async_send_request_op(
  AsyncStream&           stream,
  std::string_view const host,
  std::string_view const port,
  RequestSerializer      serializer,
  ResponseParser&        parser,
  Buffer&                buffer
) -> awaitable<void>
{
  static_assert(
    boost::beast::is_async_stream<AsyncStream>::value,
    "Supplied AsyncStream does not meet type requirements");

  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  using asio::ip::tcp;

  auto token = co_await this_coro::token();

  auto resolver      = tcp::resolver(stream.get_executor().context());
  auto resolved_ips  = co_await resolver.async_resolve(host, port, token);

  co_await asio::async_connect(stream, resolved_ips, token);
  co_await http::async_write(stream, serializer, token);
  co_await http::async_read(stream, buffer, parser, token);
}

} // detail

template <
  typename RequestSerializer,
  typename ResponseParser,
  typename Buffer,
  typename CompletionToken
>
auto async_send_request(
  boost::asio::io_context& io,
  std::string_view const   host,
  std::string_view const   port,
  RequestSerializer        serializer,
  ResponseParser&          parser,
  Buffer&                  buffer,
  CompletionToken&&        token
) -> BOOST_ASIO_INITFN_RESULT_TYPE(
  CompletionToken, void(boost::system::error_code))
{
  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  using asio::ip::tcp;
  using boost::system::error_code;

  asio::async_completion<CompletionToken, void(error_code)> init(token);

  auto executor = asio::get_associated_executor(init.completion_handler, io);

  co_spawn(
    executor,
    [
      &io, &parser, &buffer, host, port,
      sr = std::move(serializer),
      handler = std::move(init.completion_handler)
    ]
    (void) mutable -> awaitable<void>
    {
      try {
        auto stream = tcp::socket(io);

        co_await detail::async_send_request_op(
          stream, host, port, sr, parser, buffer);

        stream.shutdown(tcp::socket::shutdown_send);
      }
      catch(error_code const& ec) {
        co_return handler(ec);
      }
    },
    detached);

  return init.result.get();
}

} // proto
} // foxy