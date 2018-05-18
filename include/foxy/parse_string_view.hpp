// https://www.boost.org/doc/libs/1_67_0/libs/spirit/doc/html/spirit/advanced/customize/assign_to/assign_to_attribute_from_iterators.html

#ifndef FOXY_PARSE_STRING_VIEW_HPP_
#define FOXY_PARSE_STRING_VIEW_HPP_

#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include "foxy/string_view.hpp"

template <>
struct boost::spirit::traits::assign_to_attribute_from_iterators<
  foxy::string_view,
  foxy::string_view::iterator
>
{
  static
  void call(
    foxy::string_view::iterator const& first,
    foxy::string_view::iterator const& last,
    foxy::string_view&                 attr)
  {
    attr = foxy::string_view(first, last - first);
  }
};

#endif // FOXY_PARSE_STRING_VIEW_HPP_