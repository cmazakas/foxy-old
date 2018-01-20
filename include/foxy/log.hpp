#ifndef FOXY_LOG_HPP_
#define FOXY_LOG_HPP_

#include <boost/system/error_code.hpp>

namespace foxy
{
auto fail(
  boost::system::error_code const& ec,
  char const* const what) -> void;
}

#endif // FOXY_LOG_HPP_