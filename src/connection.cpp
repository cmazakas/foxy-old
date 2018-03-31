#include "foxy/connection.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <chrono>

namespace foxy
{

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

  // ideally, this is only true in the case when socket.close()
  // is called by the timeout routine
  // we execute this branch unconditionally and before re-entering
  // the main coroutine
  // this is to help mitigate errors in the timeout routine that may
  // lead to double invocations of the user's handler
  //
  if (ec == boost::asio::error::operation_aborted || is_closed_) {
    return;
  }

  reenter(conn_coro_)
  {
    // we elect to immediately begin execution upon the strand
    // this makes it easier to use the connection in the correct
    // way, considering that we active the timer as well
    //
    yield asio::post(
      make_stranded(
        [self = this->shared_from_this()]
        (void) -> void { self->run({}, 0); }));

    timer_.expires_after(std::chrono::seconds(30));
    timeout();

    yield http::async_read_header(
      socket_, buffer_, parser_,
      make_stranded(
        [self = this->shared_from_this()]
        (error_code const& ec, std::size_t const bytes_transferred) -> void
        {
          self->run(ec, bytes_transferred);
        }));

    handler_(ec, parser_, shared_from_this());
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
        return handler_(ec, parser_, shared_from_this());
      }

      if (std::chrono::steady_clock::now() > timer_.expiry()) {
        close();
        is_closed_ = true;
        return handler_(
          boost::asio::error::basic_errors::timed_out,
          parser_,
          shared_from_this());
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