#include "foxy/client.hpp"
#include "foxy/coroutine.hpp"

#include <string>
#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>

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

  auto const* const host    = "www.google.com";
  auto const* const port    = "80";
  auto const        version = 11; // HTTP/1.1

  auto request = http::request<http::empty_body>(http::verb::get, "/", version);

  auto response = http::response<http::string_body>();

  co_await foxy::async_send_request(
    io, host, port,
    std::move(request), response,
    token);

  CHECK(response.result_int() == 200);
  CHECK(response.body().size() > 0);
}

} // anonymous


TEST_CASE("Our HTTP client")
{
  SECTION("should be able to callout to google")
  {
    asio::io_context io;
    foxy::co_spawn(io, [&]() { return make_req(io); }, foxy::detached);
    io.run();
  }
}