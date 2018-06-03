#include "foxy/client.hpp"
#include "foxy/coroutine.hpp"

#include <string>
#include <iostream>

#include <boost/core/ignore_unused.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http.hpp>

#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>

#include <catch.hpp>

namespace asio  = boost::asio;
namespace http  = boost::beast::http;
namespace beast = boost::beast;

namespace
{

auto make_req_with_allocator(
  asio::io_context& io
) -> foxy::awaitable<void, asio::io_context::executor_type>
{
  auto token = co_await foxy::this_coro::token();

  auto const* const host    = "www.google.com";
  auto const* const port    = "80";
  auto const        version = 11; // HTTP/1.1

  auto request = http::request<http::empty_body>(http::verb::get, "/", version);

  boost::container::pmr::unsynchronized_pool_resource pool;

  using allocator_type = boost::container::pmr::polymorphic_allocator<char>;

  using res_body_type = http::basic_string_body<
    char,
    std::char_traits<char>,
    allocator_type>;

  boost::beast::basic_flat_buffer<allocator_type> buffer(allocator_type(std::addressof(pool)));

  http::response_parser<res_body_type, allocator_type>
    parser(
      std::piecewise_construct,
      std::make_tuple(allocator_type(std::addressof(pool))),
      std::make_tuple(allocator_type(std::addressof(pool))));

  auto serializer = http::request_serializer<http::empty_body>(request);

  co_await foxy::async_send_request(
      io, host, port,
      std::move(serializer), parser, buffer,
      token);

  auto response = parser.release();
  CHECK(response.body().size() > 0);
  CHECK(response.result_int() == 200);
}

} // anonymous


TEST_CASE("Our allocator-aware HTTP client")
{
  SECTION("support allocator-awareness")
  {
    asio::io_context io;
    foxy::co_spawn(
      io, [&]() { return make_req_with_allocator(io); }, foxy::detached);

    io.run();
  }
}