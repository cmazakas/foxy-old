#ifndef FOXY_ASYNC_READ_BODY_HPP_
#define FOXY_ASYNC_READ_BODY_HPP_

#include <utility>
#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/parser.hpp>

#include <boost/beast/core/handler_ptr.hpp>

#include "foxy/log.hpp"
#include "foxy/type_traits.hpp"
#include "foxy/header_parser.hpp"

namespace foxy
{
namespace detail
{
template <
  typename AsyncReadStream,
  typename DynamicBuffer,
  typename SteadyTimer,
  typename Allocator,
  typename Body,
  typename Handler
>
struct read_body_op : public boost::asio::coroutine
{
public:
  using input_parser_type =
    foxy::header_parser<Allocator>;

  using output_parser_type =
    boost::beast::http::request_parser<Body, Allocator>;

private:
  struct state
  {
    AsyncReadStream&   stream;
    DynamicBuffer&     buffer;
    SteadyTimer&       timer;
    output_parser_type parser;

    state(void)         = delete;
    state(state&&)      = delete;
    state(state const&) = delete;

    state(
      Handler&,
      AsyncReadStream&    stream_,
      DynamicBuffer&      buffer_,
      SteadyTimer&        timer_,
      input_parser_type&& parser_)
    : stream(stream_)
    , buffer(buffer_)
    , timer(timer_)
    , parser(std::move(parser_))
    {
    }
  };

  boost::beast::handler_ptr<state, Handler> p_;

public:
  read_body_op(void)                = delete;

  read_body_op(read_body_op&&)      = default;
  read_body_op(read_body_op const&) = default;

  template <typename DeducedHandler>
  read_body_op(
    DeducedHandler&&    handler,
    AsyncReadStream&    stream,
    DynamicBuffer&      buffer,
    SteadyTimer&        timer,
    input_parser_type&& parser)
  : p_(
    std::forward<DeducedHandler>(handler),
    stream,
    buffer,
    timer,
    std::move(parser))
  {
  }

  // necessary Asio hooks

  // allocator-awareness
  //
  using allocator_type = boost::asio::associated_allocator_t<Handler>;

  auto get_allocator(void) const noexcept -> allocator_type
  {
    return boost::asio::get_associated_allocator(p_.handler());
  }

  // executor-awareness
  //
  using executor_type = boost::asio::associated_executor_t<
    Handler,
    decltype(std::declval<AsyncReadStream&>().get_executor())
  >;

  auto get_executor(void) const noexcept -> executor_type
  {
    return boost::asio::get_associated_executor(
      p_.handler(),
      p_->stream.get_executor());
  }

#include <boost/asio/yield.hpp>
  // main coroutine of async operation
  //
  auto operator()(
    boost::system::error_code const ec                = {},
    std::size_t               const bytes_transferred = 0
  ) -> void
  {
    namespace asio = boost::asio;
    namespace http = boost::beast::http;

    using boost::system::error_code;

    auto& p = *p_;
    reenter(*this)
    {
      while (!p.parser.is_done()) {
        if (ec) {
          return fail(ec, "parsing message body");
        }

        yield http::async_read_some(
          p.stream, p.buffer, p.parser, std::move(*this));

        // At this resume point, we don't check for any errors as:
        // > The octets should be removed by calling consume on the dynamic
        // > buffer after the read completes, regardless of any error.
        //
        p.buffer.consume(bytes_transferred);
      }

      if (ec && ec != http::error::end_of_stream) {
        return fail(ec, "parsing message body");
      }

      auto msg = p.parser.release();
      p_.invoke(boost::system::error_code{}, std::move(msg));
    }
  }
#include <boost/asio/unyield.hpp>
};
} // detail

/**
 * async_read_body
 *
 * Asynchronously read a request body given a complete
 * `foxy::header_parser` object. The supplied message handler
 * must be invokable with the following signature:
 * void(error_code, http::request<Body, basic_fields<Allocator>>&&)
 */
template <
  typename Body,
  typename AsyncReadStream,
  typename DynamicBuffer,
  typename SteadyTimer,
  typename Allocator,
  typename MessageHandler
>
auto async_read_body(
  AsyncReadStream&                 stream,
  DynamicBuffer&                   buffer,
  SteadyTimer&                     timer,
  foxy::header_parser<Allocator>&& parser,
  MessageHandler&&                 handler
) -> BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
  void(
    boost::system::error_code,
    boost::beast::http::request<
      Body,
      boost::beast::http::basic_fields<Allocator>
    >&&))
{
  static_assert(
    is_body_v<Body> &&
    is_async_read_stream_v<AsyncReadStream>, "Type traits not met");

  namespace http = boost::beast::http;

  using handler_type =
    void(
      boost::system::error_code ec,
      http::request<Body, http::basic_fields<Allocator>>&&);

  using read_body_op_type = detail::read_body_op<
    AsyncReadStream,
    DynamicBuffer,
    SteadyTimer,
    Allocator,
    Body,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type)
  >;

  // remember async_completion only constructs with
  // lvalue ref to CompletionToken
  boost::asio::async_completion<
    MessageHandler, handler_type> init(handler);

  read_body_op_type(
    init.completion_handler,
    stream,
    buffer,
    timer,
    std::move(parser)
  )();

  return init.result.get();
}
} // foxy

#endif // FOXY_ASYNC_READ_BODY_HPP_