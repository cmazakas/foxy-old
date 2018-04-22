#ifndef FOXY_CLIENT_HPP_
#define FOXY_CLIENT_HPP_

#include <string>
#include <utility>
#include <iostream>

#include <boost/asio/post.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/system/error_code.hpp>
#include <boost/beast/http/message.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/async_result.hpp"

namespace foxy
{

namespace detail
{

template <typename Body, typename Fields, typename Handler>
auto send_request_op(Handler handler) -> awaitable<void>
{
  auto res = http::response<Body, Fields>();
  res.result(200);
  handler({},  http::response<Body, Fields>());
  co_return;
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
  typename ResBody, typename ResFields,
  typename AsyncStream,
  typename ReqBody, typename ReqFields,
  typename CompletionToken
>
auto async_send_request(
  std::string const&                              host,
  std::string const&                              port,
  AsyncStream&                                    stream,
  boost::beast::http::request<ReqBody, ReqFields> req,
  CompletionToken&&                               token
) -> init_fn_result_type<
    CompletionToken,
    void(
      boost::system::error_code,
      boost::beast::http::response<ResBody, ResFields>)
  >
{
  using boost::system::error_code;

  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  asio::async_completion<
    CompletionToken, void(
      error_code, http::response<ResBody, ResFields>)> init(token);

  co_spawn(
    stream.get_executor().context(),
    [handler = std::move(init.completion_handler)](void) mutable -> awaitable<void>
    {
      return detail::send_request_op<ResBody, ResFields>(std::move(handler));
    },
    detached);

  return init.result.get();
}

} // foxy

#endif // FOXY_CLIENT_HPP_