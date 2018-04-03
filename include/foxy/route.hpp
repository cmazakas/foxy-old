#ifndef FOXY_ROUTE_HPP_
#define FOXY_ROUTE_HPP_

#include <utility>

#include <boost/utility/string_view.hpp>

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/algorithm/query/any.hpp>

#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_parse.hpp>

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
  return boost::fusion::list<Routes...>(std::forward<Routes>(routes)...);
}

template <typename Routes>
auto match_route(
  boost::string_view const  sv,
  Routes             const& routes) -> bool
{
  namespace qi = boost::spirit::qi;

  return boost::fusion::any(
    routes,
    [sv](auto const& route) -> bool
    {
      auto const is_match = qi::parse(sv.begin(), sv.end(), route.rule);
      if (is_match) {
        // invoke handler with synthesized value
        route.handler();
      }
      return is_match;
    });
}
} // foxy

#endif // FOXY_ROUTE_HPP_