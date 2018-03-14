#include "foxy/connection.hpp"
#include <chrono>

namespace foxy
{
connection::connection(socket_type socket)
  : socket_(std::move(socket))
  , strand_(socket_.get_executor())
  , timer_(socket_.get_executor().context())
{
}

auto connection::socket(void) & noexcept -> socket_type&
{
  return socket_;
}

auto connection::buffer(void) & noexcept -> buffer_type&
{
  return buffer_;
}

auto connection::executor(void) & noexcept -> strand_type&
{
  return strand_;
}

auto connection::timer(void) & noexcept -> timer_type&
{
  return timer_;
}

#include <boost/asio/yield.hpp>
auto connection::run(
  boost::system::error_code const ec,
  std::size_t               const bytes_transferred) -> void
{
  namespace asio = boost::asio;
  namespace http = boost::beast::http;

  using boost::system::error_code;

  reenter(conn_coro_)
  {
    yield http::async_read_header(
      socket_, buffer_, parser_,
      make_stranded(
        [self = this->shared_from_this()]
        (error_code const& ec, std::size_t const bytes_transferred) -> void
        {
          if (ec) {
            return fail(ec, "header parsing");
          }
          self->run({}, bytes_transferred);
        }));

    if (handler_) {
      yield asio::post(
        make_stranded(
          [self = this->shared_from_this()]
          (void) -> void
          {
            self->handler_({}, self->parser_, self->shared_from_this());
          }));
    }

    close();
  }
}
#include <boost/asio/unyield.hpp>

#include <boost/asio/yield.hpp>
auto connection::timeout(boost::system::error_code const ec) -> void
{
  reenter(timer_coro_)
  {
    while (true) {
      if (ec && ec != boost::asio::error::operation_aborted) {
        return fail(ec, "connection timeout handling");
      }

      if (std::chrono::steady_clock::now() > timer_.expiry()) {
        return close();
      }

      yield timer_.async_wait(
        make_stranded(
          [self = this->shared_from_this()]
          (boost::system::error_code const& ec) -> void
          {
            self->timeout(ec);
          }));
    }
  }
}
#include <boost/asio/unyield.hpp>

auto connection::close(void) -> void
{
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
  socket_.close();
}

} // foxy