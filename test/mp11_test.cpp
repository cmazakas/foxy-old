#include <boost/beast/core/string.hpp>

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_parse.hpp>

#include <string>
#include <utility>
#include <cstddef>
#include <iostream>

#include "foxy/route.hpp"

#include <catch.hpp>

namespace qi     = boost::spirit::qi;
namespace beast  = boost::beast;
namespace fusion = boost::fusion;

namespace
{
template <typename ...Ts>
auto make_list(Ts&&... ts)
{
  return fusion::list<Ts...>{std::forward<Ts>(ts)...};
}

template <typename ...Rules>
auto apply_rules(
  beast::string_view     const  sv,
  fusion::list<Rules...> const& rule_list) -> void
{
  fusion::for_each(
    rule_list,
    [=](auto const& rule) -> void
    {
      std::cout << std::boolalpha << qi::parse(sv.begin(), sv.end(), rule) << '\n';
    });
}
}

TEST_CASE("Random MP11 stuff")
{
  SECTION("should hopefully compile")
  {
    auto int_rule = qi::rule<char const*>{qi::int_};
    auto dbl_rule = qi::rule<char const*>{qi::double_};

    auto const sv        = beast::string_view{"5"};
    auto const rule_list = make_list(int_rule, dbl_rule);

    REQUIRE(qi::parse(sv.begin(), sv.end(), int_rule));
    REQUIRE(qi::parse(sv.begin(), sv.end(), dbl_rule));

    apply_rules(sv, rule_list);
  }
}