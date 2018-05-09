#include "foxy/route.hpp"
#include "foxy/match_route.hpp"

#include <boost/asio/executor.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

#include <catch.hpp>

namespace qi   = boost::spirit::qi;
namespace asio = boost::asio;

TEST_CASE("Our router")
{
  SECTION("should invoke a handler upon a succesful route match")
  {
    using iterator_type = boost::string_view::iterator;

    auto was_called = false;

    auto const int_rule = qi::rule<iterator_type, int()>("/" >> qi::int_);

    auto const int_rule_handler = [&was_called](int const x) -> void
    {
      was_called = true;
      REQUIRE(x == 1337);
    };

    auto const routes = foxy::make_routes(
      foxy::make_route(int_rule, int_rule_handler));

    auto const* target = "/1337";

    auto executor = asio::executor();

    REQUIRE(foxy::match_route(target, routes, executor));
    REQUIRE(was_called);
  }

  SECTION("should _not_ invoke a handler upon a failed route match")
  {
    using iterator_type = boost::string_view::iterator;

    auto was_called = false;

    auto const routes = foxy::make_routes(
      foxy::make_route(
        qi::rule<iterator_type>("/" >> qi::int_),
        [&was_called](void) -> void
        {
          was_called = true;
        }));

    auto const* target = "/rawr";

    auto executor = asio::executor();

    REQUIRE(!foxy::match_route(target, routes, executor));
    REQUIRE(!was_called);
  }
}