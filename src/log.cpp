#include "foxy/log.hpp"
#include <iostream>

namespace foxy
{
auto fail(
  boost::system::error_code const& ec,
  boost::string_view        const  what) -> void
{
  std::cerr << what << " : " << ec.message() << '\n';
}

auto log(boost::string_view const what) -> void
{
  std::cout << what << '\n';
}
}