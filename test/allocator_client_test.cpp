#include "foxy/client.hpp"
#include "foxy/coroutine.hpp"

#include <string>
#include <iostream>
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

  using fields_type = http::basic_fields<allocator_type>;

  using header_type = http::response_header<fields_type>;

  auto allocator = allocator_type(std::addressof(pool));
  auto fields    = fields_type(allocator);
  auto header    = header_type(fields);

  http::response_parser<
    res_body_type, allocator_type
  > parser(header);

  auto serializer = http::request_serializer<http::empty_body>(request);

  try {
    std::cout << "addressof allocator is " << std::addressof(allocator) << "\n\n";
    std::cout << "address of pool is : " << allocator.resource() << '\n';

    co_await foxy::async_send_request(
      io, host, port,
      std::move(serializer), parser, allocator,
      token);

    CHECK(parser.is_done());
    CHECK(parser.is_header_done());
    CHECK(parser.chunked());
    CHECK(parser.got_some());
    CHECK(std::addressof(pool) == allocator.resource());

    std::cout << "going to test the response now...\n\n";

    auto response = parser.release();
    CHECK(response.body().size() > 0);
    CHECK(response.result_int() == 200);

    std::cout << "done with the tests now...\n\n";

  } catch (std::exception const& e) {
    std::cout << e.what() << '\n';
  }

  std::cout << "going to leave the coro now..\n\n";
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

    std::cout << "io context is now done working...\n\n";
  }
}