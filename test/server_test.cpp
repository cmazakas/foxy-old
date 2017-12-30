#include <limits>
#include <string>
#include <memory>
#include <utility>
#include <cstddef>
#include <iostream>

#include <boost/system/error_code.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/handler_ptr.hpp>

#include <catch.hpp>

namespace ip    = boost::asio::ip;
namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

using tcp = boost::asio::ip::tcp;

using boost::system::error_code;

// copy the Beast examples and create a useful error logging function
auto fail(error_code const& ec, char const* what) -> void
{
  std::cerr << what << " : " << ec.message() << "\n\n";
}

template <
  typename AsyncStream,
  typename Handler,
  typename DynamicBuffer,
  bool     isRequest,
  typename OtherBody,
  typename Allocator,
  typename Body
>
struct read_body_op : public asio::coroutine
{
public:
  using parser_type = http::parser<isRequest, Body, Allocator>;
  using other_parser_type = http::parser<isRequest, OtherBody, Allocator>;

private:
  struct state
  {
  public:
    AsyncStream&   stream;
    DynamicBuffer& buffer;
    parser_type    parser;

    state(
      Handler&,
      AsyncStream&        stream_,
      DynamicBuffer&      buffer_,
      other_parser_type&& parser_)
      : stream{stream_}
      , buffer{buffer_}
      , parser{std::move(parser_)}
    {}

    state(state&&) = default;
    state(state const&) = default;
  };

  beast::handler_ptr<state, Handler> p_;

public:

  using allocator_type = asio::associated_allocator_t<Handler>;
  using executor_type  = asio::associated_executor_t<
    Handler, decltype(std::declval<AsyncStream&>().get_executor())>;

  read_body_op(read_body_op&& op) = default;
  read_body_op(read_body_op const& op) = default;

  template <typename DeducedHandler>
  read_body_op(
    DeducedHandler&&    handler,
    AsyncStream&        stream,
    DynamicBuffer&      buffer,
    other_parser_type&& parser)
    : p_{
      std::forward<DeducedHandler>(handler),
      stream, buffer,
      std::move(parser)}
  {}

  auto operator()(
    error_code  const ec = {},
    std::size_t const bytes_transferred = 0) -> void;

  auto get_allocator(void) const noexcept -> allocator_type
  {
    return asio::get_associated_allocator(p_.handler());
  }

  auto get_executor(void) const noexcept -> executor_type
  {
    return asio::get_associated_executor(
      p_.handler(), p_->stream.get_executor());
  }
};

#include <boost/asio/yield.hpp>
template <
  typename AsyncStream,
  typename Handler,
  typename DynamicBuffer,
  bool     isRequest,
  typename OtherBody,
  typename Allocator,
  typename Body
>
auto read_body_op<
  AsyncStream, Handler, DynamicBuffer, isRequest, OtherBody, Allocator, Body
>::operator()(
  error_code  const ec,
  std::size_t const bytes_transferred
) -> void
{
  auto& p = *p_;

  reenter (*this) {
    yield http::async_read(p.stream, p.buffer, p.parser, std::move(*this));

    if (ec) {
      fail(ec, "read body op");
    } else {
      p_.invoke(error_code{}, p.parser.release());
    }
  }
}
#include <boost/asio/unyield.hpp>

/**
 * async_read_body
 *
 * Initiates the asynchronous operation
 *
 * We opt-in to the boost::asio::async_result set of traits
 * to customize our initiator
 * */
template <
  typename Body,
  typename AsyncReadStream,
  typename DynamicBuffer,
  bool     isRequest,
  typename OtherBody,
  typename Allocator,
  typename MessageHandler
>
auto async_read_body(
  AsyncReadStream&                     stream,
  DynamicBuffer&                       buffer,
  http::parser<
    isRequest, OtherBody, Allocator>&& parser,
  MessageHandler&&                     handler
) -> BOOST_ASIO_INITFN_RESULT_TYPE(
  MessageHandler,
  void(
    error_code,
    http::message<isRequest, Body, http::basic_fields<Allocator>>&&))
{
  using handler_type = void(error_code, http::message<isRequest, Body, http::basic_fields<Allocator>>&&);

  using read_body_op_t = read_body_op<
    AsyncReadStream,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type),
    DynamicBuffer,
    isRequest, OtherBody, Allocator,
    Body
  >;

  asio::async_completion<MessageHandler, handler_type> init{handler};
  read_body_op_t{
    init.completion_handler,
    stream,
    buffer,
    std::move(parser)
  }();
  return init.result.get();
}

struct session
  : public asio::coroutine
  , public std::enable_shared_from_this<session>
{
private:
  using executor_type = asio::io_context::executor_type;

  tcp::socket                 socket_;
  asio::strand<executor_type> strand_;
  beast::flat_buffer          buffer_;

  http::request_parser<http::empty_body> header_parser_;

public:
  explicit
  session(tcp::socket socket)
    : socket_{std::move(socket)}
    , strand_{socket_.get_executor()}
  {}

  template <typename F>
  auto make_stranded(F&& f)
  {
    return asio::bind_executor(strand_, std::forward<F>(f));
  }

#include <boost/asio/yield.hpp>
  auto run(
    error_code const ec = {}) -> void
  {
    std::cout << "Beginning session...\n\n";

    reenter(*this) {

      yield http::async_read_header(
        socket_, buffer_, header_parser_,
        make_stranded(
          [self = shared_from_this()](
            error_code  const& ec, std::size_t const bytes_transferred) -> void
          {
            std::cout << "Read in header at " << bytes_transferred << " bytes\n\n";
            self->run(ec);
          }
        ));

      yield async_read_body<http::string_body>(
        socket_, buffer_,
        std::move(header_parser_),
        make_stranded(
          [self = shared_from_this()]
          (error_code ec, http::message<true, http::string_body>&& message) -> void
          {
            std::cout << "Finished parsing request\n\n";

            if (ec) { fail(ec, "read body"); }

            auto m = http::request<http::string_body>{std::move(message)};

            auto res = http::response<http::string_body>{http::status::ok, m.version()};
            res.set(http::field::content_type, "text/plain");
            res.body() = "Received the following payload: \"" + m.body() + "\"";
            res.prepare_payload();

            http::write(self->socket_, res);

            self->socket_.shutdown(tcp::socket::shutdown_both);
          }
        ));

    }
  }
#include <boost/asio/unyield.hpp>
};

struct listener
  : public asio::coroutine
  , public std::enable_shared_from_this<listener>
{
private:
  tcp::acceptor acceptor_;
  tcp::socket   socket_;

public:
  listener(
    asio::io_context&    ioc,
    tcp::endpoint const& endpoint)
  : acceptor_{ioc, endpoint}
  , socket_{ioc}
  {}

#include <boost/asio/yield.hpp>
  // we rely on the implicit strain
  auto run(error_code const ec = {}) -> void
  {
    reenter(*this) {
      yield acceptor_.async_accept(
        socket_,
        [self = shared_from_this()](auto const ec) {
          std::cout << "Completed acceptance\n\n";
          self->run(ec);
        });

      if (ec) { fail(ec, "accept"); return; }

      std::make_shared<session>(std::move(socket_))->run();
    }
  }
#include <boost/asio/unyield.hpp>
};

TEST_CASE("Our HTTP listener")
{
  SECTION("should at some point in time compile")
  {
    asio::io_context ioc{};

    auto const addr = std::string{"127.0.0.1"};
    auto const port = static_cast<unsigned short>(1337);

    std::make_shared<listener>(
      ioc, tcp::endpoint{ip::make_address_v4(addr), port}
    )->run();

    std::cout << "Server is up and running\n\n";
    ioc.run();
  }
}