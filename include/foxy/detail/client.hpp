#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/type_traits.hpp>
#include "foxy/coroutine.hpp"

namespace foxy
{
namespace proto
{

template <
  typename AsyncStream,
  typename RequestSerializer,
  typename ResponseParser,
  typename Buffer
>
auto async_send_request(
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

  co_await asio::async_connect(
    stream,
    resolved_ips.begin(), resolved_ips.end(),
    token);

  co_await http::async_write(stream, serializer, token);
  co_await http::async_read(stream, buffer, parser, token);
}

} // proto
} // foxy