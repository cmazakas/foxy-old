#ifndef FOXY_MATCH_ROUTE_HPP_
#define FOXY_MATCH_ROUTE_HPP_

#include <tuple>
#include <utility>

#include <boost/hof/unpack.hpp>
#include <boost/hof/partial.hpp>

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
template <typename Rule, typename Handler, typename ...Args>
auto invoke_with_no_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler,
  Args&&...          args
) -> bool
{
  namespace qi = boost::spirit::qi;

  auto const is_match = qi::parse(sv.begin(), sv.end(), rule);
  if (is_match) {
    handler(args...);
  }
  return is_match;
}

template <typename Rule, typename Handler, typename ...Args>
auto invoke_with_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler,
  Args&&...          args
) -> bool
{
  namespace qi = boost::spirit::qi;

  using rule_type = std::decay_t<decltype(rule)>;
  using sig_type  = typename rule_type::sig_type;
  using attr_type = boost::callable_traits::return_type_t<sig_type>;

  attr_type  attr;
  auto const is_match = qi::parse(sv.begin(), sv.end(), rule, attr);
  if (is_match) {
    handler(attr, args...);
  }
  return is_match;
}

template <typename ...Args>
struct match_and_invoke
{
  template <typename Route>
  auto operator()(
    string_view const  sv,
    Route       const& route,
    Args...            args) const -> bool
  {
    auto const& rule    = route.rule;
    auto const& handler = route.handler;

    using rule_type           = std::decay_t<decltype(rule)>;
    using sig_type            = typename rule_type::sig_type;
    using has_void_return     =
      boost::callable_traits::has_void_return<sig_type>;
    using has_non_void_return = negation<has_void_return>;

    if constexpr (has_void_return::value) {
      return detail::invoke_with_no_attr(
        sv, rule, handler, args...);
    } else {
      return detail::invoke_with_attr(
        sv, rule, handler, args...);
    }
  }
};

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
template <
  typename RouteSequence,
  typename ...Args
>
auto match_route(
  string_view   const  sv,
  RouteSequence const& routes,
  Args...              args
) -> bool
{
  namespace hof    = boost::hof;
  namespace fusion = boost::fusion;

    auto matcher = hof::partial(
      detail::match_and_invoke<Args...>());

  return fusion::any(
    routes,
    [sv, matcher, arg_tuple = std::make_tuple(args...)]
    (auto const& route) -> bool
    {
      if constexpr (std::tuple_size_v<decltype(arg_tuple)> == 0) {
        return matcher(sv, route);
      } else {
        return hof::unpack(matcher(sv, route))(arg_tuple);
      }
    });
}

} // foxy

#endif // FOXY_MATCH_ROUTE_HPP_