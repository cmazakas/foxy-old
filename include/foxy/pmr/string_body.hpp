#ifndef FOXY_PMR_STRING_BODY_HPP_
#define FOXY_PMR_STRING_BODY_HPP_

#include <string>
#include <boost/beast/http/string_body.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace foxy
{
namespace pmr
{

template <
  typename CharT,
  typename Traits = std::char_traits<CharT>
>
using basic_string_body =
  boost::beast::http::basic_string_body<
    CharT,
    Traits,
    boost::container::pmr::polymorphic_allocator<CharT>
  >;

} // pmr
} // foxy

#endif // FOXY_PMR_STRING_BODY_HPP_