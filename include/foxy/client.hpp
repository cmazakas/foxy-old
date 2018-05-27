#ifndef FOXY_CLIENT_HPP_
#define FOXY_CLIENT_HPP_

#include <utility>
#include <string_view>

#include <boost/core/ignore_unused.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/core/flat_buffer.hpp>

#include <boost/beast/http.hpp>

#include "foxy/coroutine.hpp"

/**
 * TODO:
 * 1. refactor code to respect HTTP status code 100
 * 2. refactor code to respect HTTP status codes guaranteeing no body via
 *    http::parser::skip
 * 3. decide how to handle redirects
 */
namespace foxy
{

namespace detail
{

template <
  typename AsyncStream,
  typename ReqBody, typename ReqFields,
  typename ResBody, typename ResFields,
  typename Handler
>
auto send_request_op(
  AsyncStream                                       stream,
  std::string_view const                            host,
  std::string_view const                            port,
  boost::beast::http::request<ReqBody, ReqFields>   request,
  boost::beast::http::response<ResBody, ResFields>& response,
  Handler                                           handler
) -> awaitable<void>
{
  using boost::asio::ip::tcp;

  namespace asio  = boost::asio;
  namespace http  = boost::beast::http;
  namespace beast = boost::beast;

  auto ec          = boost::system::error_code();
  auto token       = co_await this_coro::token();
  auto error_token = make_redirect_error_token(token, ec);

  auto resolver = tcp::resolver(stream.get_executor().context());

  auto const results = co_await resolver.async_resolve(host, port, error_token);
  if (ec) {
    co_return handler(ec);
  }

  boost::ignore_unused(co_await asio::async_connect(
    stream,
    results.begin(), results.end(),
    error_token));

  if (ec) {
    co_return handler(ec);
  }

  boost::ignore_unused(
    co_await http::async_write(stream, request, error_token));
  if (ec) {
    co_return handler(ec);
  }

  auto buffer   = beast::flat_buffer();

  boost::ignore_unused(
    co_await http::async_read(stream, buffer, response, error_token));

  if (ec) {
    co_return handler(ec);
  }

  stream.shutdown(tcp::socket::shutdown_send, ec);
  if (ec) {
    co_return handler(ec);
  }

  handler({});
}

} // detail

/**
 * Asynchronously write the `http::request` through the supplied `stream`,
 * invoking the supplied handler with (error_code, http::response) upon
 * completion
 *
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation
 * of the handler will be performed in a manner equivalent to using
 * boost::asio::io_context::post.
 */
template <
  typename ReqBody, typename ReqFields,
  typename ResBody, typename ResFields,
  typename CompletionToken
>
auto async_send_request(
  boost::asio::io_context&                          io,
  std::string_view const                            host,
  std::string_view const                            port,
  boost::beast::http::request<ReqBody, ReqFields>   request,
  boost::beast::http::response<ResBody, ResFields>& response,
  CompletionToken&&                                 token
) -> BOOST_ASIO_INITFN_RESULT_TYPE(
  CompletionToken, void(boost::system::error_code))
{
  using boost::system::error_code;

  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  asio::async_completion<CompletionToken, void(error_code)> init(token);

  auto executor = asio::get_associated_executor(init.completion_handler, io);

  co_spawn(
    executor,
    [
      &io,
      host, port,
      req = std::move(request),
      &response,
      handler = std::move(init.completion_handler)
    ]
    (void) mutable -> awaitable<void>
    {
      return detail::send_request_op(
        boost::asio::ip::tcp::socket(io),
        host, port,
        std::move(req),
        response,
        std::move(handler));
    },
    detached);

  return init.result.get();
}

} // foxy

#endif // FOXY_CLIENT_HPP_