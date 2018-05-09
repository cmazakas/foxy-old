#include "foxy/listener.hpp"

#include <string>

#include <boost/asio/io_context.hpp>

#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include "foxy/route.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/async_read_body.hpp"

#include <catch.hpp>

namespace qi    = boost::spirit::qi;
namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

using boost::asio::ip::tcp;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    using strand_type = boost::asio::strand<
      boost::asio::io_context::executor_type>;

    asio::io_context io;

    auto const int_rule = qi::rule<char const*, int()>("/" >> qi::int_);

    auto const routes = foxy::make_routes(
      foxy::make_route(
        int_rule,
        [](
          boost::system::error_code const& ec,
          tcp::socket&                     stream,
          foxy::header_parser<>&           parser,
          int const                        user_id) -> foxy::awaitable<void, strand_type>
        {
          auto token = co_await foxy::this_coro::token();

          auto res = http::response<http::string_body>();
          res.body() = "Your user id is : " + std::to_string(user_id) + "\n";
          res.prepare_payload();

          co_await http::async_write(stream, res, token);

          stream.shutdown(tcp::socket::shutdown_both);
          stream.close();

          co_return;
        }
      ));

    std::cout << "launching server...\n";

    foxy::co_spawn(
      io,
      [&]()
      {
        std::cout << "calling listener function...\n";

        return foxy::listener(
          io,
          tcp::endpoint(asio::ip::address_v4({127, 0, 0, 1}), 1337),
          routes);
      },
      foxy::detached);

    io.run();
    std::cout << "io context is done working now\n";
  }
}