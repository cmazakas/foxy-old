#ifndef FOXY_SESSION_HPP_
#define FOXY_SESSION_HPP_

#include <memory>
#include <utility>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/asio/ip/tcp.hpp>

#include "foxy/coroutine.hpp"

namespace foxy
{

struct session : public std::enable_shared_from_this<session>
{
private:
  boost::asio::ip::tcp::socket              socket_;
  boost::asio::strand<
    boost::asio::io_context::executor_type> strand_;

public:
  explicit
  session(boost::asio::ip::tcp::socket socket)
  : socket_(std::move(socket))
  , strand_(socket_.get_executor())
  {
  }

  auto start(void) -> void
  {
    co_spawn(
      socket_.get_executor(),
      []() -> awaitable<void>
      {
        auto token    = co_await this_coro::token();
        auto executor = co_await this_coro::executor();
        co_spawn(
          executor,
          []() -> awaitable<void> { co_return; },
          token);

        co_return;
      },
      detached);
  }
};

} // foxy

#endif // FOXY_SESSION_HPP_