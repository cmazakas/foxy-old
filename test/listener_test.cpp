#include <boost/asio/io_context.hpp>

#include <boost/beast/http/string_body.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_parse.hpp>

#include <string>
#include <memory>

#include "foxy/listener.hpp"
#include "foxy/route.hpp"

#include <catch.hpp>

namespace qi    = boost::spirit::qi;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
namespace asio  = boost::asio;
namespace ip    = boost::asio::ip;

using tcp = asio::ip::tcp;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    asio::io_context ioc{};

    auto const addr = std::string{"127.0.0.1"};
    auto const port = static_cast<unsigned short>(1337);

    auto int_rule = qi::rule<char const*>{qi::int_};

    auto routes = foxy::make_routes(
      foxy::make_route<http::string_body>(int_rule, [](){}));

    auto const endpoint = tcp::endpoint{ip::make_address_v4(addr), port};

    std::make_shared<foxy::listener<decltype(routes)>>(
      ioc, endpoint, routes
    )->run();

    std::cout << "Server is up and running\n\n";
    ioc.run();
  }
}