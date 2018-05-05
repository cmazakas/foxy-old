#include "foxy/listener.hpp"

#include <boost/asio/io_context.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

#include "foxy/route.hpp"
#include "foxy/coroutine.hpp"

#include <catch.hpp>

namespace qi   = boost::spirit::qi;
namespace asio = boost::asio;

using boost::asio::ip::tcp;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    asio::io_context io;

    auto const int_rule = qi::rule<char const*, int()>("/" >> qi::int_);

    auto const routes = foxy::make_routes(
      foxy::make_route(
        int_rule,
        [](
          boost::system::error_code const& ec,
          tcp::socket&                     stream,
          foxy::header_parser<>&           parser,
          int const                        user_id) -> void
        {

        }
      ));

    foxy::co_spawn(
      io,
      [&]()
      {
        return foxy::listener(
          io,
          { tcp::v4(), static_cast<unsigned short>(1337) },
          routes);
      },
      foxy::detached);

    io.run();
  }
}