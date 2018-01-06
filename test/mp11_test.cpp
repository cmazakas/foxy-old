#include <boost/beast/core/string.hpp>
#include <boost/beast/http/string_body.hpp>

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
#include <type_traits>

#include "foxy/route.hpp"
#include "foxy/make_route.hpp"

#include <catch.hpp>

namespace qi     = boost::spirit::qi;
namespace beast  = boost::beast;
namespace fusion = boost::fusion;
namespace http   = boost::beast::http;

namespace
{
template <typename Routes>
auto apply_rules(
  beast::string_view const  sv,
  Routes const& routes) -> void
{
  fusion::for_each(
    routes,
    [=](auto const& route) -> void
    {
      using route_type = std::decay_t<decltype(route)>;
      using body_type = typename route_type::body_type;
      // std::cout << std::boolalpha << qi::parse(sv.begin(), sv.end(), rule) << '\n';
    });
}
}

TEST_CASE("Random MP11 stuff")
{
  SECTION("should hopefully compile")
  {
    auto int_rule = qi::rule<char const*>{qi::int_};
    auto dbl_rule = qi::rule<char const*>{qi::double_};

    auto const sv     = beast::string_view{"5"};
    auto const routes = foxy::make_routes(
      foxy::make_route<http::string_body>(int_rule, [](){}));

    REQUIRE(qi::parse(sv.begin(), sv.end(), int_rule));
    REQUIRE(qi::parse(sv.begin(), sv.end(), dbl_rule));

    apply_rules(sv, routes);
  }
}