#ifndef FOXY_ASYNC_READ_BODY_HPP_
#define FOXY_ASYNC_READ_BODY_HPP_

#include <utility>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>

#include <boost/beast/core/handler_ptr.hpp>

#include "foxy/log.hpp"
#include "foxy/type_traits.hpp"

namespace foxy
{
namespace detail
{
template <
  typename AsyncReadStream,
  typename DynamicBuffer,
  typename InBody,
  typename Allocator,
  typename OutBody,
  typename Handler
>
struct read_body_op : public boost::asio::coroutine
{
public:
  using input_parser_type =
    boost::beast::http::request_parser<InBody, Allocator>;

  using output_parser_type =
    boost::beast::http::request_parser<OutBody, Allocator>;

private:
  struct state;
  boost::beast::handler_ptr<state, Handler> p_;

  struct state
  {
    AsyncReadStream&   stream;
    DynamicBuffer&     buffer;
    output_parser_type parser;

    state(void)         = delete;
    state(state&&)      = default;
    state(state const&) = default;

    state(
      Handler&,
      AsyncReadStream&    stream_,
      DynamicBuffer&      buffer_,
      input_parser_type&& parser_)
    : stream{stream_}
    , buffer{buffer_}
    , parser{std::move(parser_)}
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
      if (!p.parser.is_done()) {
        if (ec) {
          return fail(ec, "parsing message body");
        }

        yield http::async_read_some(
          p.stream, p.buffer, p.parser, std::move(*this));
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
 * Asynchronously read a request body, taking a parser
 * containg the header of the request and then switching
 * the body type from InBody to OutBody and invoking
 * the supplied handler with:
 * (error_code, http::request<OutBody, basic_fields<Allocator>>&&)
 */
template <
  typename OutBody,
  typename AsyncReadStream,
  typename DynamicBuffer,
  typename InBody,
  typename Allocator,
  typename MessageHandler
>
auto async_read_body(
  AsyncReadStream&                    stream,
  DynamicBuffer&                      buffer,
  boost::beast::http::request_parser<
    InBody, Allocator>&&              parser,
  MessageHandler&&                    handler
) -> BOOST_ASIO_INITFN_RESULT_TYPE(
  MessageHandler,
  void(
    boost::system::error_code,
    boost::beast::http::request<
      OutBody, boost::beast::http::basic_fields<Allocator>>&&))
{
  static_assert(
    is_async_read_stream_v<AsyncReadStream>, "Type traits not met");

  using fields_type  = boost::beast::http::basic_fields<Allocator>;
  using request_type = boost::beast::http::request<OutBody, fields_type>;
  using handler_type =
    void(boost::system::error_code ec, request_type&&);

  using read_body_op_type = detail::read_body_op<
    AsyncReadStream,
    DynamicBuffer,
    InBody,
    Allocator,
    OutBody,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type)
  >;

  // remember async_completion only constructs with
  // lvalue ref to CompletionToken
  boost::asio::async_completion<
    MessageHandler, handler_type> init{handler};

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