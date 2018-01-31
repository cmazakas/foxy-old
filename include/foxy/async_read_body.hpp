#ifndef FOXY_ASYNC_READ_BODY_HPP_
#define FOXY_ASYNC_READ_BODY_HPP_

#include <chrono>
#include <utility>
#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/basic_deadline_timer.hpp>

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
  struct state;
  boost::beast::handler_ptr<state, Handler> p_;

  struct state
  {
    using timer_type = boost::asio::basic_deadline_timer<std::chrono::seconds>;

    AsyncReadStream&   stream;
    DynamicBuffer&     buffer;
    output_parser_type parser;
    // timer_type         timer;

    state(void)         = delete;
    state(state&&)      = delete;
    state(state const&) = default;

    state(
      Handler&,
      AsyncReadStream&    stream_,
      DynamicBuffer&      buffer_,
      input_parser_type&& parser_)
    : stream{stream_}
    , buffer{buffer_}
    , parser{std::move(parser_)}
    // , timer{stream.get_executor()}
    {
      // timer.expires_from_now(std::chrono::seconds{30});
      // timer.async_wait(
      //   boost::asio::bind_executor(
      //     stream.get_executor()));
    }
  };

public:
  read_body_op(void)                = delete;
  read_body_op(read_body_op&&)      = default;
  read_body_op(read_body_op const&) = default;

  template <typename DeducedHandler>
  read_body_op(
    DeducedHandler&&    handler,
    AsyncReadStream&    stream,
    DynamicBuffer&      buffer,
    input_parser_type&& parser)
  : p_{
    std::forward<DeducedHandler>(handler),
    stream,
    buffer,
    std::move(parser)}
  {
  }

  // necessary Asio hooks

  // allocator-awareness
  using allocator_type = boost::asio::associated_allocator_t<Handler>;

  auto get_allocator(void) const noexcept -> allocator_type
  {
    return boost::asio::get_associated_allocator(p_.handler());
  }

  // executor-awareness
  using executor_type = boost::asio::associated_executor_t<
    Handler,
    decltype(std::declval<AsyncReadStream&>().get_executor())
  >;

  auto get_executor(void) const noexcept -> executor_type
  {
    return boost::asio::get_associated_executor(
      p_.handler(), p_->stream.get_executor());
  }

  // main coroutine of async operation
#include <boost/asio/yield.hpp>
  auto operator()(
    boost::system::error_code const ec  = {},
    std::size_t const bytes_transferred = 0
  ) -> void
  {
    namespace http = boost::beast::http;

    auto& p = *p_;
    reenter(*this) {
      while (!p.parser.is_done()) {
        if (ec) {
          return fail(ec, "parsing message body");
        }

        yield http::async_read_some(
          p.stream, p.buffer, p.parser, std::move(*this));

        p.buffer.consume(bytes_transferred);
      }

      if (ec && ec != http::error::end_of_stream) {
        return fail(ec, "parsing message body");
      }

      p_.invoke(boost::system::error_code{}, p.parser.release());
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
  typename Connection,
  typename Allocator,
  typename MessageHandler
>
auto async_read_body(
  Connection& connection,
  foxy::header_parser<Allocator>&& parser,
  MessageHandler&& handler
) -> BOOST_ASIO_INITFN_RESULT_TYPE(
  MessageHandler,
  void(
    boost::system::error_code,
    boost::beast::http::request<
      Body, boost::beast::http::basic_fields<Allocator>>&&))
{
  using async_read_stream_type =
    std::decay<decltype(connection.socket())>::type;

  using dynamic_buffer_type =
    std::decay<decltype(connection.buffer())>::type;

  static_assert(
    is_async_read_stream_v<async_read_stream_type>, "Type traits not met");

  namespace http = boost::beast::http;

  using request_type = http::request<Body, http::basic_fields<Allocator>>;
  using handler_type =
    void(boost::system::error_code ec, request_type&&);

  using read_body_op_type = detail::read_body_op<
    async_read_stream_type,
    dynamic_buffer_type,
    Allocator,
    Body,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type)
  >;

  // remember async_completion only constructs with
  // lvalue ref to CompletionToken
  boost::asio::async_completion<
    MessageHandler, handler_type> init{handler};

  auto& stream = connection.socket();
  auto& buffer = connection.buffer();

  read_body_op_type{
    init.completion_handler,
    stream,
    buffer,
    std::move(parser)
  }();

  return init.result.get();
}
} // foxy

#endif // FOXY_ASYNC_READ_BODY_HPP_