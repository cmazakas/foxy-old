#ifndef FOXY_ROUTE_HPP_
#define FOXY_ROUTE_HPP_

#include <utility>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/fusion/container/vector.hpp>

#include "foxy/type_traits.hpp"

namespace foxy
{
template <
  typename Iterator, typename A1, typename A2, typename A3,
  typename Handler
>
struct route
{
  using rule_type = boost::spirit::qi::rule<Iterator, A1, A2, A3>;

  rule_type rule;
  Handler   handler;
};

template <
  typename Iterator, typename A1, typename A2, typename A3,
  typename Handler
>
auto make_route(
  boost::spirit::qi::rule<Iterator, A1, A2, A3> const& rule,
  Handler&& handler
) -> route<Iterator, A1, A2, A3, Handler>
{
  return {rule, std::forward<Handler>(handler)};
}

template <typename ...Routes>
auto make_routes(Routes&&... routes)
{
  return boost::fusion::vector<Routes...>(std::forward<Routes>(routes)...);
}

} // foxy

#endif // FOXY_ROUTE_HPP_