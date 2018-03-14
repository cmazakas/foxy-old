#ifndef FOXY_CONNECTION_HPP_
#define FOXY_CONNECTION_HPP_

#include <memory>
#include <cstddef>
#include <utility>
#include <iostream>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/core/flat_buffer.hpp>

#include "foxy/log.hpp"
#include "foxy/header_parser.hpp"
#include "foxy/async_read_body.hpp"

namespace foxy
{
struct connection : public std::enable_shared_from_this<connection>
{
public:
  using socket_type        = boost::asio::ip::tcp::socket;
  using buffer_type        = boost::beast::flat_buffer;
  using strand_type        =
    boost::asio::strand<boost::asio::io_context::executor_type>;

  using timer_type         = boost::asio::steady_timer;
  using coroutine_type     = boost::asio::coroutine;
  using header_parser_type = header_parser<>;

private:
  socket_type socket_;
  strand_type strand_;
  buffer_type buffer_;
  timer_type  timer_;

  coroutine_type conn_coro_;
  coroutine_type timer_coro_;

  header_parser_type parser_;

  template <typename F>
  auto make_stranded(F&& f)
  {
    return boost::asio::bind_executor(strand_, std::forward<F>(f));
  }

public:
  connection(socket_type socket);

  auto socket(void)   & noexcept -> socket_type&;
  auto buffer(void)   & noexcept -> buffer_type&;
  auto executor(void) & noexcept -> strand_type&;
  auto timer(void)    & noexcept -> timer_type&;

  auto run(
    boost::system::error_code const  ec = {},
    std::size_t               const bytes_transferred = 0) -> void;

  auto timeout(boost::system::error_code const ec = {}) -> void;
  auto close(void) -> void;
};

} // foxy

#endif // FOXY_CONNECTION_HPP_