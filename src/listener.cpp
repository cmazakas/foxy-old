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
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include "foxy/make_v4_endpoint.hpp"

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
    tcp::socket                      socket_;
    io::strand                       strand_;
    beast::flat_buffer               buffer_;
    http::request<http::string_body> request_;

  public:
    http_session(tcp::socket&& socket)
      : socket_{std::move(socket)}
      , strand_{socket_.get_io_service()}
      , buffer_{}
    {}

#include <boost/asio/yield.hpp>
    auto respond(
      boost::system::error_code ec = {},
      std::size_t const         bytes_transferred = 0) -> void
    {
      reenter (*this) {
        yield http::async_read(
          socket_, buffer_, request_, strand_.wrap(
          [self = shared_from_this()](auto const ec, auto const bytes_transferred) -> void
          {
            self->respond(ec, bytes_transferred);
          }));

        // handle error case here
        auto res = std::make_shared<http::response<http::string_body>>(200, BOOST_BEAST_VERSION_STRING);
        res->body() = "rawr\n\n";
        res->prepare_payload();
        yield http::async_write(socket_, *res, );
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
    listener(io::io_service& ios, tcp::endpoint const endpoint)
      : endpoint_{endpoint}
      , acceptor_{ios, endpoint}
      , socket_{ios}
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