#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>

namespace foxy
{
struct listener
  : public boost::asio::coroutine
  , public std::enable_shared_from_this<listener>
{
private:
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket   socket_;

public:
  listener(
    boost::asio::io_context&              ioc,
    boost::asio::ip::tcp::endpoint const& endpoint);

  auto run(boost::system::error_code const ec = {}) -> void;
};
} // foxy


#endif // FOXY_LISTENER_HPP_