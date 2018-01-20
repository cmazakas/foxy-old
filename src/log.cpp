#include "foxy/log.hpp"
#include <iostream>

namespace foxy
{
auto fail(
  boost::system::error_code const& ec,
  char const* const what) -> void
{
  std::cerr << what << " : " << ec.message() << '\n';
}
}