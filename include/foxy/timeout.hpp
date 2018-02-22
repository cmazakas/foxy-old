#ifndef FOXY_TIMEOUT_HPP_
#define FOXY_TIMEOUT_HPP_

#include <boost/asio/coroutine.hpp>
#include <boost/system/error_code.hpp>

namespace foxy
{
template <typename Stream>
struct shutdown_timer : public boost::asio::coroutine
{
  auto operator()(boost::system::error_code const ec = {}) -> void
  {

  }
};
}

#endif // FOXY_TIMEOUT_HPP_