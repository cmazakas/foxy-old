#ifndef FOXY_MATCH_ROUTE_HPP_
#define FOXY_MATCH_ROUTE_HPP_

#include <boost/hof/if.hpp>
#include <boost/hof/eval.hpp>
#include <boost/hof/first_of.hpp>

#include <boost/spirit/include/qi_parse.hpp>

#include <boost/fusion/algorithm/query/any.hpp>

#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_void_return.hpp>

#include "foxy/string_view.hpp"
#include "foxy/type_traits.hpp"

namespace foxy
{
namespace detail
{
template <typename Rule, typename Handler>
auto invoke_with_no_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler
) -> bool
{
  namespace qi = boost::spirit::qi;

  auto const is_match = qi::parse(sv.begin(), sv.end(), rule);
  if (is_match) {
    handler();
  }
  return is_match;
}

template <typename Rule, typename Handler>
auto invoke_with_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler
) -> bool
{
  namespace qi = boost::spirit::qi;

  using rule_type = std::decay_t<decltype(rule)>;
  using sig_type  = typename rule_type::sig_type;
  using attr_type = boost::callable_traits::return_type_t<sig_type>;

  attr_type  attr;
  auto const is_match = qi::parse(sv.begin(), sv.end(), rule, attr);
  if (is_match) {
    handler(attr);
  }
  return is_match;
}

template <typename Route>
auto match_and_invoke(
  string_view const  sv,
  Route       const& route
) -> bool
{
  namespace hof = boost::hof;

  // lol non-TMP code
  //
  auto const& rule    = route.rule;
  auto const& handler = route.handler;

  // lol TMP
  //
  using rule_type           = std::decay_t<decltype(rule)>;
  using sig_type            = typename rule_type::sig_type;
  using has_void_return     = boost::callable_traits::has_void_return<sig_type>;
  using has_non_void_return = negation<has_void_return>;

  return hof::eval(
    hof::first_of(
      hof::if_(has_void_return())(
        [&, sv](auto const id) -> bool
        {
          return detail::invoke_with_no_attr(sv, id(rule), id(handler));
        }),
      hof::if_(has_non_void_return())(
        [&, sv](auto const id) -> bool
        {
          return detail::invoke_with_attr(sv, id(rule), id(handler));
        })
    ));
}
} // detail

/**
 * match_route
 *
 * match route takes an input string_view and a Fusion
 * ForwardSequence of foxy::route types.
 *
 * The sequence of routes is probed until a route's rule
 * returns positive when the string_view is parsed
 *
 * Upon successful match, the route's associated handler
 * is invoked and the function returns
 */
template <typename RouteSequence>
auto match_route(
  string_view   const  sv,
  RouteSequence const& routes
) -> void
{
  namespace fusion = boost::fusion;

  fusion::any(
    routes,
    [sv](auto const& route) -> bool
    {
      return detail::match_and_invoke(sv, route);
    });
}

} // foxy

#endif // FOXY_MATCH_ROUTE_HPP_