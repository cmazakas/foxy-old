#ifndef FOXY_MATCH_ROUTE_HPP_
#define FOXY_MATCH_ROUTE_HPP_


#include <utility>

#include <boost/hof/partial.hpp>
#include <boost/asio/is_executor.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/fusion/algorithm/query/any.hpp>

#include <boost/callable_traits/return_type.hpp>
#include <boost/callable_traits/has_void_return.hpp>

#include "foxy/coroutine.hpp"
#include "foxy/string_view.hpp"
#include "foxy/type_traits.hpp"

namespace foxy
{
namespace detail
{
template <typename Rule, typename Handler, typename Executor, typename ...Args>
auto invoke_with_no_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler,
  Executor& executor,
  Args&&...          args
) -> bool
{
  namespace qi = boost::spirit::qi;

  using handler_return_type = boost::callable_traits::return_type_t<Handler>;

  auto const is_match = qi::parse(sv.begin(), sv.end(), rule);
  if (is_match) {
    if constexpr (is_awaitable_v<handler_return_type>) {

      co_spawn(
        executor,
        [=]() { return handler(args...); },
        detached);
    } else {
      handler(std::forward<Args>(args)...);
    }
  }
  return is_match;
}

template <typename Rule, typename Handler, typename Executor, typename ...Args>
auto invoke_with_attr(
  string_view const  sv,
  Rule        const& rule,
  Handler     const& handler,
  Executor& executor,
  Args&&...          args
) -> bool
{
  namespace qi = boost::spirit::qi;

  using rule_type = std::decay_t<decltype(rule)>;
  using sig_type  = typename rule_type::sig_type;
  using attr_type = boost::callable_traits::return_type_t<sig_type>;

  using handler_return_type = boost::callable_traits::return_type_t<Handler>;

  attr_type  attr;
  auto const is_match = qi::parse(sv.begin(), sv.end(), rule, attr);
  if (is_match) {
    if constexpr (is_awaitable_v<handler_return_type>) {

      co_spawn(
        executor,
        [=]() { return handler(args..., attr); },
        detached);
    } else {
      handler(std::forward<Args>(args)..., attr);
    }
  }
  return is_match;
}

template <typename Executor, typename ...Args>
struct match_and_invoke
{
  Executor& executor;

  match_and_invoke(Executor& executor_)
  : executor(executor_)
  {
  }

  template <typename Route>
  auto operator()(
    string_view const  sv,
    Route       const& route,
    Args&&...          args) const -> bool
  {
    auto const& rule    = route.rule;
    auto const& handler = route.handler;

    using rule_type           = std::decay_t<decltype(rule)>;
    using sig_type            = typename rule_type::sig_type;
    using has_void_return     =
      boost::callable_traits::has_void_return<sig_type>;

    if constexpr (has_void_return::value) {
      return detail::invoke_with_no_attr(
        sv, rule, handler, executor, std::forward<Args>(args)...);
    } else {
      return detail::invoke_with_attr(
        sv, rule, handler, executor, std::forward<Args>(args)...);
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
  typename Executor,
  typename ...Args
>
auto match_route(
  string_view   const  sv,
  RouteSequence const routes,
  Executor& executor,
  Args&&...            args
) -> bool
{
  static_assert(
    boost::asio::is_executor<Executor>::value,
    "Type traits of Executor requirement not met");

  namespace hof    = boost::hof;
  namespace fusion = boost::fusion;

  auto matcher = hof::partial(detail::match_and_invoke<Executor, Args...>(executor));

  return fusion::any(
    routes,
    [
      sv, matcher, &executor,
      &args...
    ]
    (auto const& route) -> bool
    {
      if constexpr (sizeof...(args) == 0) {
        return matcher(sv, route);
      } else {
        return matcher(sv, route)(std::forward<Args>(args)...);
      }
    });
}

} // foxy

#endif // FOXY_MATCH_ROUTE_HPP_