#include "foxy/connection.hpp"

#include <boost/asio/bind_executor.hpp>

#include <boost/beast/core/string.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include "foxy/async_read_body.hpp"

using boost::system::error_code;

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

#include <boost/asio/yield.hpp>
auto foxy::connection::run(
  error_code  const ec,
  std::size_t const bytes_transferred,
  std::shared_ptr<
    http::request_parser<
      http::empty_body>>  header_parser) -> void
{
  reenter (*this) {
    yield {
      auto header_parser =
        std::make_shared<http::request_parser<http::empty_body>>();

      http::async_read_header(
        socket_, buffer_, *header_parser,
        asio::bind_executor(
          strand_,
          [
            self   = shared_from_this(),
            parser = header_parser
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