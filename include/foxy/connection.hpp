#ifndef FOXY_CONNECTION_HPP_
#define FOXY_CONNECTION_HPP_

#include <memory>
#include <cstddef>

#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>

#include <boost/beast/core/flat_buffer.hpp>

namespace foxy
{
struct connection
  : public boost::asio::coroutine
  , std::enable_shared_from_this<connection>
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
  explicit
  connection(socket_type socket)
    : socket_{std::move(socket)}
    , strand_{socket_.get_executor()}
  {
  }

  auto run(
    boost::system::error_code const       ec = {},
    std::size_t               const       bytes_transferred = 0,
    std::shared_ptr<
      boost::beast::http::request_parser<
        boost::beast::http::empty_body>>  header_parser = nullptr) -> void;
};

} // foxy

#endif // FOXY_CONNECTION_HPP_