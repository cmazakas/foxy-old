#ifndef FOXY_TYPE_TRAITS_HPP_
#define FOXY_TYPE_TRAITS_HPP_

#include <type_traits>
#include <boost/beast/http/type_traits.hpp>
#include <boost/beast/core/type_traits.hpp>

namespace foxy
{
template <typename Stream>
constexpr
bool const
is_async_read_stream_v = boost::beast::is_async_read_stream<Stream>::value;

template <typename Handler, typename Signature>
constexpr
bool const
is_completion_handler_v = boost::beast::is_completion_handler<Handler, Signature>::value;

template <typename Body>
constexpr
bool const
is_body_v = boost::beast::http::is_body<Body>::value;

// ripped straight from cppreference
//
template <bool B>
using bool_constant = std::integral_constant<bool, B>;

template <typename B>
struct negation : bool_constant<!bool(B::value)> { };

template <typename B>
constexpr
bool const negation_v = negation<B>::value;
} // foxy

#endif // FOXY_TYPE_TRAITS_HPP_