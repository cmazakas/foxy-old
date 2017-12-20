#include <memory>
#include <utility>
#include <iostream>

#include <boost/system/error_code.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/beast/core/flat_buffer.hpp>

namespace io    = boost::asio;
namespace beast = boost::beast;
namespace http  = boost::beast::http;

using tcp = boost::asio::ip::tcp;

namespace foxy
{
  struct http_session
    : public io::coroutine
    , public std::enable_shared_from_this<http_session>
  {
  private:
    io::strand<
      io::io_context::executor_type>       strand_;
    tcp::socket                            socket_;
    beast::flat_buffer                     buffer_;
    http::request_parser<http::empty_body> header_parser_;

  public:
    http_session(tcp::socket&& socket)
      : socket_{std::move(socket)}
      , strand_{socket_.get_executor()}
      , buffer_{}
      , header_parser_{}
    {}

#include <boost/asio/yield.hpp>
    auto respond(
      boost::system::error_code ec = {},
      std::size_t const         bytes_transferred = 0) -> void
    {
      reenter (*this) {
        // we initially only want to read in the headers of the request
        yield http::async_read_header(
          socket_, buffer_, header_parser_,
          [self = shared_from_this()](
            boost::system::error_code const& ec,
            std::size_t               const  num_bytes) -> void
          {
            self->respond(ec, num_bytes);
          });


      }
    }
  };
#include <boost/asio/unyield.hpp>

  struct listener
    : public io::coroutine
    , public std::enable_shared_from_this<listener>
  {
  private:
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;
    tcp::socket   socket_;

  public:
    listener(io::io_context& ioc, tcp::endpoint const endpoint)
      : endpoint_{endpoint}
      , acceptor_{ioc, endpoint}
      , socket_{ioc}
    {}
#include <boost/asio/yield.hpp>
    auto listen(boost::system::error_code ec = {}) -> void
    {
      reenter(*this) {
        for (;;) {
          yield acceptor_.async_accept(
            socket_, endpoint_,
            [self = shared_from_this()](auto&& ec) -> void
            {
              self->listen(std::forward<decltype(ec)>(ec));
            });

          if (ec) { continue; }

          auto session = std::make_shared<http_session>(std::move(socket_));
          session->respond();
        }
      }
    }
#include <boost/asio/unyield.hpp>
  };
}