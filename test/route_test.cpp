#include "foxy/route.hpp"

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

#include <catch.hpp>

namespace qi = boost::spirit::qi;

TEST_CASE("Our router")
{
  SECTION("should hopefully compile")
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

    auto const* target = "/";


  }
}