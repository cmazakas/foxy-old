#ifndef FOXY_TYPE_TRAITS_HPP_
#define FOXY_TYPE_TRAITS_HPP_

#include <type_traits>
#include <boost/beast/core/type_traits.hpp>

namespace foxy
{
template <typename Stream>
constexpr
bool const is_async_read_stream_v = boost::beast::is_async_read_stream<Stream>::value;

template <typename Handler, typename Signature>
constexpr
bool const is_completion_handler_v = boost::beast::is_completion_handler<Handler, Signature>::value;
} // foxy

#endif // FOXY_TYPE_TRAITS_HPP_