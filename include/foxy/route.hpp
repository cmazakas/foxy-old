#ifndef FOXY_ROUTE_HPP_
#define FOXY_ROUTE_HPP_

#include "foxy/type_traits.hpp"

#include <utility>

#include <boost/hof/if.hpp>
#include <boost/hof/first_of.hpp>

#include <boost/utility/string_view.hpp>

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/algorithm/query/any.hpp>

#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_parse.hpp>

#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_void_return.hpp>

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
  namespace qi  = boost::spirit::qi;
  namespace hof = boost::hof;

  return boost::fusion::any(
    routes,
    [sv](auto const& route) -> bool
    {
      auto const& rule    = route.rule;
      auto const& handler = route.handler;

      using rule_type = typename std::decay<decltype(rule)>::type;
      using sig_type  = typename rule_type::sig_type;

      using has_void_return = boost::callable_traits::has_void_return<sig_type>;
      using synth_type      = boost::callable_traits::return_type_t<sig_type>;

      using has_non_void_return = negation<has_void_return>;

      return hof::first_of(
        hof::if_(has_void_return())([&rule, &handler, sv](void) -> bool
        {
          auto const is_match = qi::parse(sv.begin(), sv.end(), rule);
          if (is_match) {
            handler();
          }
          return is_match;
        }),
        hof::if_(has_non_void_return())([&rule, &handler, sv](void) -> bool
        {
          return false;
        })
      )();
    });
}
} // foxy

#endif // FOXY_ROUTE_HPP_