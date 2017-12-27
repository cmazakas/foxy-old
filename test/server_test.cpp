#include <string>
#include <memory>
#include <utility>
#include <iostream>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/parser.hpp>
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
  bool isRequest, typename Body, typename Allocator
>
struct read_op
{
private:

  struct state : public asio::coroutine
  {
  public:
    AsyncStream&       stream;
    beast::flat_buffer buffer;

    http::parser<isRequest, Body, Allocator> parser;

    explicit
    state(Handler& handler, AsyncStream& stream_)
      : stream{stream_}
    {}
  };

  beast::handler_ptr<state, Handler> ptr_;

public:
  template <typename DeducedHandler>
  read_op(AsyncStream& stream, DeducedHandler&& handler)
    : ptr_{std::forward<DeducedHandler>(handler), stream}
  {}


  auto operator()(
    error_code  const ec,
    std::size_t const bytes_transferred) -> void;
};

#include <boost/asio/yield.hpp>
template <
  typename AsyncStream, typename Handler,
  bool isRequest, typename Body, typename Allocator>
auto read_op<AsyncStream, Handler, isRequest, Body, Allocator>::operator()(
  error_code  const ec,
  std::size_t const bytes_transferred
) -> void
{
  auto& ptr = *ptr_;

  reenter (ptr) {
    yield http::async_read(
      ptr.stream, ptr.buffer, ptr.parser, std::move(*this));


  }
}
#include <boost/asio/unyield.hpp>

template<
  typename AsyncReadStream,
  typename DynamicBuffer,
  bool isRequest, typename Body, typename Allocator,
  typename MessageHandler
>
auto async_read_body(
  AsyncReadStream&                           stream,
  DynamicBuffer&                             buffer,
  http::parser<isRequest, Body, Allocator>&& parser,
  MessageHandler&&                           handler)
-> BOOST_ASIO_INITFN_RESULT_TYPE(
  MessageHandler,
  void(
    error_code,
    http::message<isRequest, Body, http::basic_fields<Allocator>>&&))
{
  using handler_type = void(error_code, http::message<isRequest, Body, http::basic_fields<Allocator>>&&);

  auto init = asio::async_completion<MessageHandler, handler_type>{handler};

  read_op<
    AsyncReadStream,
    BOOST_ASIO_HANDLER_TYPE(MessageHandler, handler_type),
    isRequest, Body, Allocator>{}();

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

public:
  explicit
  session(tcp::socket socket)
    : socket_{std::move(socket)}
    , strand_{socket_.get_executor()}
  {}

#include <boost/asio/yield.hpp>
  auto run(
    error_code const ec = {}) -> void
  {
    reenter(*this) {
      // yield http::async_read_header(
      //   socket_, buffer_,
      // );
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
  // note, we don't use strands here because we actually want
  // our acceptance function to be reentrant by multiple threads
  auto run(error_code const ec = {}) -> void
  {
    reenter(*this) {
      yield acceptor_.async_accept(
        socket_,
        [self = shared_from_this()](auto const ec) { self->run(ec); });

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

    ioc.run();
  }
}