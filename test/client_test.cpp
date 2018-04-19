#include <string>
#include <boost/asio/io_context.hpp>

#include "foxy/client.hpp"

#include <catch.hpp>

using boost::asio::experimental::co_spawn;
using boost::asio::experimental::detached;

namespace this_coro = boost::asio::experimental::this_coro;
namespace asio      = boost::asio;

namespace
{

template <typename T>
using awaitable = boost::asio::experimental::awaitable<
  T, boost::asio::io_context::executor_type>;

auto make_req() -> awaitable<void>
{
  auto token = co_await this_coro::token();

  auto const host   = std::string("www.google.com");
  auto const port   = std::string("443");
  auto const target = std::string("/");

  foxy::send_request(host, port, target, token);
}

}

TEST_CASE("Our HTTP client")
{
  SECTION("should hopefully compile")
  {
    asio::io_context io;



  }
}