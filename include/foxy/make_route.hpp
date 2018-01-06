#ifndef FOXY_MAKE_ROUTE_HPP_
#define FOXY_MAKE_ROUTE_HPP_

#include <boost/fusion/container/list.hpp>
#include <boost/spirit/include/qi_rule.hpp>

#include <utility>

#include "foxy/route.hpp"

namespace foxy
{
template <
  typename Body,
  typename Iterator, typename A1, typename A2, typename A3,
  typename Handler
>
auto make_route(
  boost::spirit::qi::rule<Iterator, A1, A2, A3> const& rule,
  Handler&& handler)
{
  return route<
    Body,
    Iterator, A1, A2, A3,
    Handler
  >{
    rule,
    std::forward<Handler>(handler)
  };
}

template <typename ...Routes>
auto make_routes(Routes&&... routes)
{
  return boost::fusion::list<Routes...>{std::forward<Routes>(routes)...};
}
} // foxy

#endif // FOXY_MAKE_ROUTE_HPP_