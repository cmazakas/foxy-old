#include <string>
#include <memory>

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
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_char_class.hpp>

#include "foxy/route.hpp"
#include "foxy/listener.hpp"
#include "foxy/header_parser.hpp"

#include <catch.hpp>

namespace ip    = boost::asio::ip;
namespace qi    = boost::spirit::qi;
namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

using tcp = asio::ip::tcp;
using boost::system::error_code;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    asio::io_context io;

    auto const addr = std::string("127.0.0.1");
    auto const port = static_cast<unsigned short>(1337);

    auto const int_rule  = qi::rule<char const*>("/" >> qi::int_);
    auto const name_rule = qi::rule<char const*, std::string()>("/" >> +qi::alpha);

    auto const int_handler =
      [](
        error_code const                  ec,
        foxy::header_parser<>&            parser,
        std::shared_ptr<foxy::connection> conn
      ) -> void
      {
        using body_type = http::string_body;

        auto str_parser = http::request_parser<body_type>(std::move(parser));
        http::read(conn->socket(), conn->buffer(), str_parser);

        auto req = str_parser.release();

        auto res = http::response<http::string_body>(http::status::ok, 11);
        res.body() =
          "Received the following content: " + req.body();

        res.prepare_payload();

        http::write(conn->socket(), res);
        conn->run(ec);
      };

    auto const name_handler =
      [](
        error_code const ec,
        http::request_parser<http::empty_body>&,
        auto conn,
        std::string const name) -> void
      {
        auto res = http::response<http::string_body>{http::status::ok, 11};
        res.body() = name;
        res.prepare_payload();
        http::write(conn->get_socket(), res);
        conn->run(ec);
      };

    auto routes = foxy::make_routes(
      foxy::make_route(int_rule, int_handler),
      foxy::make_route(name_rule, name_handler));

    auto const endpoint = tcp::endpoint{ip::make_address_v4(addr), port};

    std::make_shared<foxy::listener<decltype(routes)>>(
      ioc, endpoint, routes
    )->run();

    std::cout << "Server is up and running\n\n";
    ioc.run();
  }
}