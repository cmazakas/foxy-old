#include <boost/beast/http/read.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/http/string_body.hpp>

#include "foxy/connection.hpp"
#include "foxy/async_read_body.hpp"

using boost::system::error_code;

namespace asio = boost::asio;
namespace http = boost::beast::http;

#include <boost/asio/yield.hpp>
auto foxy::connection::run(
  error_code  const ec = {},
  std::size_t const bytes_transferred = 0,
  std::shared_ptr<
    http::request_parser<http::empty_body>> header_parser = nullptr) -> void
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
            parser = std::move(header_parser)
          ](
            error_code const ec,
            std::size_t const bytes_transferred
          ) -> void
          {
            self->run(ec, bytes_transferred, std::move(parser));
          }));
    }

    // default implementation for now
    yield foxy::async_read_body<http::string_body>(
      socket_,
      buffer_,
      std::move(*header_parser),
      asio::bind_executor(
        strand_,
        [self = shared_from_this()]
        (error_code ec, http::request<http::string_body> request)
        {

        }));
  }
}
#include <boost/asio/unyield.hpp>