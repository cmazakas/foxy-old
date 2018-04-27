#ifndef FOXY_LISTENER_HPP_
#define FOXY_LISTENER_HPP_

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

#include "foxy/coroutine.hpp"

namespace foxy
{

auto listener(
  boost::asio::io_context&              io,
  boost::asio::ip::tcp::endpoint const& endpoint
) -> awaitable<void>;

} // foxy

#endif // FOXY_LISTENER_HPP_