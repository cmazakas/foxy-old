#include "foxy/client.hpp"
#include "foxy/coroutine.hpp"

#include <string>
#include <iostream>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/beast/core/ostream.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <catch.hpp>

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

using asio::ip::tcp;

namespace
{

auto make_req(tcp::socket& stream) -> foxy::awaitable<void, asio::io_context::executor_type>
{
  auto token = co_await foxy::this_coro::token();

  auto const host   = std::string("www.google.com");
  auto const port   = std::string("443");
  auto const target = std::string("/");

  auto req = http::request<http::empty_body>(http::verb::get, target, 11);

  std::cout << "made request, invoking initiating function\n";

  auto res = co_await foxy::async_send_request<http::string_body, http::fields>(
    host, port,
    stream,
    req,
    token);

  std::cout << "send request is done\n";

  std::cout << res.result() << '\n';
}

}


TEST_CASE("Our HTTP client")
{
  SECTION("should hopefully compile")
  {
    asio::io_context io;

    tcp::socket stream(io);

    foxy::co_spawn(io, [&]() { return make_req(stream); }, foxy::detached);
    io.run();
  }
}