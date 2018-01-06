#ifndef FOXY_ROUTE_HPP_
#define FOXY_ROUTE_HPP_

#include <boost/spirit/include/qi_rule.hpp>
#include <utility>

namespace foxy
{
template <
  typename Body,
  typename Iterator, typename A1, typename A2, typename A3,
  typename Handler
>
struct route
{
  using rule_type = boost::spirit::qi::rule<Iterator, A1, A2, A3>;

  rule_type rule;
  Handler   handler;
};
}

#endif // FOXY_ROUTE_HPP_