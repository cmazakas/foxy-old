#pragma once

#include <boost/beast/core/type_traits.hpp>
#include "foxy/coroutine.hpp"

namespace foxy
{
namespace proto
{

template <
  typename AsyncStream
>
auto async_send_request(
  AsyncStream& stream
) -> foxy::awaitable<void>
{
  static_assert(
    boost::beast::is_async_stream<AsyncStream>::value,
    "Supplied AsyncStream does not meet type requirements");
}

} // proto
} // foxy