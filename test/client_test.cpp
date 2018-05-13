#include "foxy/client.hpp"
#include "foxy/coroutine.hpp"

#include <string>
#include <iostream>

#include <boost/asio/io_context.hpp>

#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <catch.hpp>

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

namespace
{

auto make_req(
  asio::io_context& io
) -> foxy::awaitable<void, asio::io_context::executor_type>
{
  auto token = co_await foxy::this_coro::token();

  auto const host = std::string("www.google.com");
  auto const port = std::string("80");

  auto req = http::request<http::empty_body>(http::verb::get, "/", 11);
  auto res = co_await foxy::async_send_request<http::string_body, http::fields>(
    io,
    host, port,
    req,
    token);

  REQUIRE(res.result_int() == 200);
  REQUIRE(res.body().size() > 0);
}

}


TEST_CASE("Our HTTP client")
{
  SECTION("should hopefully compile")
  {
    asio::io_context io;
    foxy::co_spawn(io, [&]() { return make_req(io); }, foxy::detached);
    io.run();
  }
}