#include <chrono>
#include <algorithm>
#include <type_traits>

namespace foxy
{
template <
  typename ForwardIterator,
  typename Rep,
  typename Period
>
auto timeout_connections(
  ForwardIterator begin,
  ForwardIterator end,
  std::chrono::duration<Rep, Period> const& duration) -> void
{
  static_assert(
    std::is_convertible<
      std::chrono::time_point<
        std::chrono::steady_clock,
        std::chrono::duration<Rep, Period>
      >,
      decltype((*begin).last_activity())
    >::value,
    "Iterator dereference type must support \".last_activity(void)\" returning a time_point");

  auto const remover = [&duration](auto const& conn) -> bool
  {
    auto const now = std::chrono::steady_clock::now();
    if (!conn.io_pending()) {
      return false;
    }
    return (now - conn.last_activity() >= duration);
  };

  auto timed_out_begin = std::remove_if(begin, end, remover);

  std::for_each(
    timed_out_begin, end,
    [](auto& conn) -> void
    {
      conn.close();
    });
}
}