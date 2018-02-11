#ifndef FOXY_ASYNC_READ_BODY_HPP_
#define FOXY_ASYNC_READ_BODY_HPP_

#include <chrono>
#include <utility>
#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/steady_timer.hpp>
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
auto make_empty_function(void)
{
  return [](boost::system::error_code const&){};
}

using empty_function_t = decltype(make_empty_function());

template <typename Timer, typename WaitHandler, typename ReadOp>
struct async_timer_op : public boost::asio::coroutine
{
  struct state
  {
    WaitHandler wait_handler;
    Timer&      timer;
    unsigned&   curr_op_count;
    unsigned&   prev_op_count;
    ReadOp&     read_op;

    state(void)         = delete;
    state(state const&) = default;
    state(state&&)      = default;

    state(
      WaitHandler&& wait_handler_,
      Timer& timer_,
      unsigned& curr_op_count_,
      unsigned& prev_op_count_,
      ReadOp&   read_op_)
    : wait_handler(std::move(wait_handler_))
    , timer(timer_)
    , curr_op_count(curr_op_count_)
    , prev_op_count(prev_op_count_)
    , read_op(read_op_)
    {
    }
  };

  std::shared_ptr<state> p_;

  async_timer_op(void)                  = delete;
  async_timer_op(async_timer_op const&) = default;
  async_timer_op(async_timer_op&&)      = default;

  template <typename DeducedHandler>
  async_timer_op(
    DeducedHandler&& wait_handler,
    Timer&        timer,
    unsigned&     curr_op_count,
    unsigned&     prev_op_count,
    ReadOp&       read_op)
  : p_(std::make_shared<state>(
    std::forward<DeducedHandler>(wait_handler),
    timer,
    curr_op_count,
    prev_op_count,
    read_op))
  {
  }

#include <boost/asio/yield.hpp>
  auto operator()(boost::system::error_code const ec = {}) -> void
  {
    namespace asio = boost::asio;

    auto& p = *p_;
    reenter(*this)
    {
      if (ec) {
        return p.wait_handler(asio::error::broken_pipe);
      }

      while (true) {
        // this means we have a dead connection
        // invoke the user's wait handler
        //
        if (p.curr_op_count > 0 && p.curr_op_count == p.prev_op_count) {
          return p.wait_handler(boost::system::error_code{});
        }

        p.prev_op_count = p.curr_op_count;
        p.timer.expires_from_now(std::chrono::seconds(30));
        p.timer.async_wait(std::move(*this));
        yield p.read_op();
      }
    }
  }
#include <boost/asio/unyield.hpp>
};

template <
  typename AsyncReadStream,
  typename DynamicBuffer,
  typename Allocator,
  typename Body,
  typename Handler,
  typename Timer,
  typename WaitHandler
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
    AsyncReadStream&   stream;
    DynamicBuffer&     buffer;
    output_parser_type parser;

    Timer       timer;
    unsigned    curr_op_count;
    unsigned    prev_op_count;

    detail::async_timer_op<
      Timer, WaitHandler, read_body_op> async_timer_op;

    state(void)         = delete;
    state(state&&)      = delete;
    state(state const&) = delete;

    state(
      Handler&,
      AsyncReadStream&    stream_,
      DynamicBuffer&      buffer_,
      input_parser_type&& parser_,
      WaitHandler&&       wait_handler_,
      read_body_op&       read_op)
    : stream(stream_)
    , buffer(buffer_)
    , parser(std::move(parser_))
    , timer(stream.get_executor().context())
    , curr_op_count(0)
    , prev_op_count(0)
    , async_timer_op(
      std::forward<WaitHandler>(wait_handler_),
      timer,
      curr_op_count,
      prev_op_count,
      read_op)
    {
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
    input_parser_type&& parser,
    WaitHandler&&       wait_handler)
  : p_(
    std::forward<DeducedHandler>(handler),
    stream,
    buffer,
    std::move(parser),
    std::forward<WaitHandler>(wait_handler),
    *this)
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
      p_.handler(),
      p_->stream.get_executor());
  }

#include <boost/asio/yield.hpp>
  // main coroutine of async operation
  auto operator()(
    boost::system::error_code const ec  = {},
    std::size_t const bytes_transferred = 0
  ) -> void
  {
    namespace asio = boost::asio;
    namespace http = boost::beast::http;

    using boost::system::error_code;

    auto& p = *p_;
    reenter(*this)
    {
      yield p.async_timer_op(error_code{});

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

        // Remember to increment the count of operations we've done here
        //
        ++p.curr_op_count;
      }

      p.timer.cancel();

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
  typename Allocator,
  typename MessageHandler,
  typename WaitHandler = detail::empty_function_t
>
auto async_read_body(
  AsyncReadStream&                 stream,
  DynamicBuffer&                   buffer,
  foxy::header_parser<Allocator>&& parser,
  MessageHandler&&                 handler,
  WaitHandler&&                    wait_handler = detail::make_empty_function()
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
    Allocator,
    Body,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type),
    boost::asio::steady_timer,
    WaitHandler
  >;

  // remember async_completion only constructs with
  // lvalue ref to CompletionToken
  boost::asio::async_completion<
    MessageHandler, handler_type> init(handler);

  read_body_op_type(
    init.completion_handler,
    stream,
    buffer,
    std::move(parser),
    std::forward<WaitHandler>(wait_handler)
  )();

  return init.result.get();
}
} // foxy

#endif // FOXY_ASYNC_READ_BODY_HPP_