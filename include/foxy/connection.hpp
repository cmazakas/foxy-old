#ifndef FOXY_CONNECTION_HPP_
#define FOXY_CONNECTION_HPP_

#include <iostream>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/beast/core/string.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include "foxy/async_read_body.hpp"

#include <memory>
#include <cstddef>

namespace foxy
{
template <typename RouteList>
struct connection
  : public boost::asio::coroutine
  , std::enable_shared_from_this<connection<RouteList>>
{
public:
  using socket_type = boost::asio::ip::tcp::socket;
  using strand_type = boost::asio::strand<
    boost::asio::io_context::executor_type>;
  using buffer_type = boost::beast::flat_buffer;

private:
  socket_type socket_;
  strand_type strand_;
  buffer_type buffer_;

public:
  connection(socket_type socket)
    : socket_{std::move(socket)}
    , strand_{socket_.get_executor()}
  {
  }

  auto run(
    boost::system::error_code const  ec = {},
    std::size_t const bytes_transferred = 0,
    std::shared_ptr<
      boost::beast::http::request_parser<
        boost::beast::http::empty_body>>  header_parser = nullptr) -> void;
};

#include <boost/asio/yield.hpp>
template <typename RouteList>
auto foxy::connection<RouteList>::run(
  boost::system::error_code const ec,
  std::size_t const bytes_transferred,
  std::shared_ptr<
    boost::beast::http::request_parser<
      boost::beast::http::empty_body>>  header_parser) -> void
{
  namespace http = boost::beast::http;
  using boost::system::error_code;

  reenter (*this) {
    yield {
      auto tmp_parser =
        std::make_shared<http::request_parser<http::empty_body>>();

      auto& p = *tmp_parser;

      http::async_read_header(
        socket_, buffer_, p,
        asio::bind_executor(
          strand_,
          [
            self   = shared_from_this(),
            parser = std::move(tmp_parser)
          ](
            error_code  const ec,
            std::size_t const bytes_transferred
          ) -> void
          {
            self->run(ec, bytes_transferred, std::move(parser));
          }));
    }

    // default implementation for now
    using body_type = http::empty_body;

    yield foxy::async_read_body<body_type>(
      socket_, buffer_, std::move(*header_parser),
      asio::bind_executor(
        strand_,
        [self = shared_from_this()]
        (error_code ec, http::request<body_type> request)
        {
          auto const target = request.target();

          auto res = http::response<http::string_body>{http::status::ok, 11};
          res.body() =
            "Received the following request-target: " +
            std::string{target.begin(), target.end()};

          res.prepare_payload();

          http::write(self->socket_, res);
          self->run(ec);
        }));

    socket_.shutdown(socket_type::shutdown_both);
  }
}
#include <boost/asio/unyield.hpp>

} // foxy

#endif // FOXY_CONNECTION_HPP_