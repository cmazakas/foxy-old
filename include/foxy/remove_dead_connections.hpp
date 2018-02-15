#ifndef FOXY_REMOVE_DEAD_CONNECTIONS_HPP_
#define FOXY_REMOVE_DEAD_CONNECTIONS_HPP_

#include <chrono>
#include <algorithm>

namespace foxy
{
template <
  typename ForwardIterator,
  typename Duration>
auto remove_dead_connections(
  ForwardIterator begin,
  ForwardIterator end,
  Duration const  duration) -> ForwardIterator
{
  auto const curr_time = std::chrono::steady_clock::now();
  return std::remove_if(
    begin, end,
    [=](auto const& connection) -> bool
    {
      return (
        !connection.is_io_pending()
        ? false
        : (curr_time >= connection.get_last_activity() + duration));
    });
}
}

#endif // FOXY_REMOVE_DEAD_CONNECTIONS_HPP_