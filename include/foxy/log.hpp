#ifndef FOXY_LOG_HPP_
#define FOXY_LOG_HPP_

#include <boost/system/error_code.hpp>
#include <boost/utility/string_view.hpp>

namespace foxy
{
auto fail(
  boost::system::error_code const& ec,
  boost::string_view        const  what) -> void;

auto log(boost::string_view const what) -> void;
}

#endif // FOXY_LOG_HPP_