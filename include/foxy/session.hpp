#ifndef FOXY_SESSION_HPP_
#define FOXY_SESSION_HPP_

#include <memory>
#include <utility>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/asio/ip/tcp.hpp>

#include "foxy/coroutine.hpp"

namespace foxy
{

struct session : public std::enable_shared_from_this<session>
{
public:
  using socket_type = boost::asio::ip::tcp::socket;
  using strand_type = boost::asio::strand<
    boost::asio::io_context::executor_type>;

private:
  socket_type socket_;
  strand_type strand_;

public:
  explicit
  session(socket_type socket);

  auto start(void) -> void;
  auto request_handler(void) -> awaitable<void, strand_type>;
  auto timeout(void) -> awaitable<void, strand_type>;
};

} // foxy

#endif // FOXY_SESSION_HPP_