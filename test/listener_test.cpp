#include <string>
#include <memory>
#include <type_traits>

#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/write.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_char_class.hpp>
#include <boost/spirit/include/qi_kleene.hpp>

#include <boost/mpl/front.hpp>

#include "foxy/listener.hpp"
#include "foxy/route.hpp"

#include <catch.hpp>

namespace qi    = boost::spirit::qi;
namespace beast = boost::beast;
namespace http  = boost::beast::http;
namespace asio  = boost::asio;
namespace ip    = boost::asio::ip;
namespace mpl   = boost::mpl;

using tcp = asio::ip::tcp;
using boost::system::error_code;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    asio::io_context ioc{};

    auto const addr = std::string{"127.0.0.1"};
    auto const port = static_cast<unsigned short>(1337);

    auto const int_rule  = qi::rule<char const*>{"/" >> qi::int_};
    auto const name_rule = qi::rule<char const*, std::string()>{"/" >> *qi::alpha};

    using rule_type = decltype(name_rule);
    using sig_type  = rule_type::sig_type;
    using synth_attribute_type = mpl::front<sig_type>::type;

    synth_attribute_type s = "rawr";
    static_assert(std::is_same_v<synth_attribute_type, std::string>, "Requirements not met");


    auto const route_handler =
      [](
        error_code const ec,
        http::request<http::string_body> request,
        auto connection,
        qi::unused_type) -> void
      {
        auto const target = request.target();

        auto res = http::response<http::string_body>{http::status::ok, 11};
        res.body() =
          "Received the following request-target: " +
          std::string{target.begin(), target.end()};

        res.prepare_payload();

        http::write(connection->get_socket(), res);
        connection->run(ec);
      };

    auto const name_handler =
      [](
        error_code const ec,
        http::request<http::empty_body> request,
        auto connection,
        std::string const name) -> void
      {
        auto res = http::response<http::string_body>{http::status::ok, 11};
        res.body() = name;
        res.prepare_payload();
        http::write(connection->get_socket(), res);
        connection->run(ec);
      };

    auto routes = foxy::make_routes(
      foxy::make_route<http::string_body>(int_rule, route_handler),
      foxy::make_route<http::empty_body>(name_rule, name_handler));

    auto const endpoint = tcp::endpoint{ip::make_address_v4(addr), port};

    std::make_shared<foxy::listener<decltype(routes)>>(
      ioc, endpoint, routes
    )->run();

    std::cout << "Server is up and running\n\n";
    ioc.run();
  }
}