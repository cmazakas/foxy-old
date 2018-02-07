#include <chrono>
#include <utility>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/system/error_code.hpp>

#include <catch.hpp>

namespace asio = boost::asio;
using boost::system::error_code;

namespace
{
struct state
{
  asio::steady_timer timer;

  state(void) = delete;
  state(asio::io_context& io)
    : timer{io}
  {
  }

  state(state&&) = default;
  ~state(void)   = default;
};
}

TEST_CASE("The Asio steady_timer")
{
  SECTION("should be moveable")
  {
    asio::io_context   io{};
    asio::steady_timer timer{io};

    asio::steady_timer timer2{std::move(timer)};

    timer2.expires_after(std::chrono::milliseconds{100});
    timer2.async_wait([](error_code const&){});

    io.run();
  }
}