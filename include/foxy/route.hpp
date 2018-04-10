#ifndef FOXY_ROUTE_HPP_
#define FOXY_ROUTE_HPP_

#include "foxy/type_traits.hpp"

#include <utility>

#include <boost/hof/if.hpp>
#include <boost/hof/eval.hpp>
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

} // foxy

#endif // FOXY_ROUTE_HPP_