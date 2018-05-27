#include <string>

#include <boost/asio/io_context.hpp>

#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

#include <boost/spirit/home/qi/detail/assign_to.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "foxy/route.hpp"
#include "foxy/client.hpp"
#include "foxy/listener.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/parse_string_view.hpp"

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

    auto const not_found_rule =
      qi::rule<char const*, foxy::string_view()>(qi::raw[*qi::char_]);

    auto const routes = foxy::make_routes(
      foxy::make_route(
        int_rule,
        [](
          boost::system::error_code const ec,
          std::shared_ptr<foxy::session> session,
          int const user_id
        ) -> foxy::awaitable<void, strand_type>
        {
          auto& s      = *session;
          auto& buffer = s.buffer();
          auto& parser = s.parser();
          auto& stream = s.stream();

          auto token = co_await foxy::this_coro::token();

          boost::ignore_unused(
            co_await http::async_read(stream, buffer, parser, token));

          auto res = http::response<http::string_body>(http::status::ok, 11);
          res.body() = "Your user id is : " + std::to_string(user_id) + "\n";
          res.prepare_payload();

          boost::ignore_unused(co_await http::async_write(stream, res, token));

          stream.shutdown(tcp::socket::shutdown_both);
          stream.close();

          co_return;
        }
      ),
      foxy::make_route(
        not_found_rule,
        [](
          boost::system::error_code const ec,
          std::shared_ptr<foxy::session> session,
          foxy::string_view const        target
        ) -> foxy::awaitable<void, strand_type>
        {
          auto& s      = *session;
          auto& buffer = s.buffer();
          auto& parser = s.parser();
          auto& stream = s.stream();

          auto token = co_await foxy::this_coro::token();

          boost::ignore_unused(
            co_await http::async_read(stream, buffer, parser, token));

          auto res =
            http::response<http::string_body>(http::status::not_found, 11);

          res.body() =
            "Could not find the following target : " +
            std::string(target.begin(), target.end()) +
            "\n";

          res.prepare_payload();

          boost::ignore_unused(
            co_await http::async_write(stream, res, token));

          stream.shutdown(tcp::socket::shutdown_both);
          stream.close();

          co_return;
        }
      ));

    foxy::co_spawn(
      io,
      [&]() -> foxy::awaitable<void>
      {
        auto token = co_await foxy::this_coro::token();

        // spawn the server independently
        //
        foxy::co_spawn(
          io,
          [&]()
          {
            return foxy::listener(
              io,
              tcp::endpoint(asio::ip::address_v4({127, 0, 0, 1}), 1337),
              routes);
          },
          foxy::detached);

        auto const* host = "127.0.0.1";
        auto const* port = "1337";

        // send two requests to our server
        //
        auto request = http::request<http::empty_body>(
          http::verb::get, "/1337", 11);

        auto response = http::response<http::string_body, http::fields>();

        co_await foxy::async_send_request(
          io, host, port, request, response, token);

        REQUIRE(response.result_int() == 200);
        REQUIRE(response.body() == "Your user id is : 1337\n");

        request = http::request<http::empty_body>(
          http::verb::get, "/abasdfasdf", 11);

        response = http::response<http::string_body, http::fields>();

        co_await foxy::async_send_request(
          io, host, port, request, response, token);

        REQUIRE(response.result_int() == 404);
        REQUIRE(
          response.body() ==
          "Could not find the following target : /abasdfasdf\n");

        // kill our server
        //
        io.stop();

        co_return;
      },
      foxy::detached);

    io.run();
  }
}