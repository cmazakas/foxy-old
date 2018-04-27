#include <vector>
#include <thread>
#include <cstddef>
#include <iostream>

#include <boost/asio/io_context.hpp>

#include "foxy/coroutine.hpp"

#include <catch.hpp>

namespace asio = boost::asio;

TEST_CASE("Our listener type")
{
  SECTION("should at least compile")
  {
    auto const num_threads = std::size_t{4};

    asio::io_context io(num_threads);

    foxy::co_spawn(
      io,
      []() -> foxy::awaitable<void>
      {
        auto token    = co_await foxy::this_coro::token();
        auto executor = co_await foxy::this_coro::executor();

        foxy::co_spawn(executor, []() -> foxy::awaitable<void> { for (int i = 0; i < 10; ++i) std::cout << "hello\n"; co_return; }, foxy::detached);
        foxy::co_spawn(executor, []() -> foxy::awaitable<void> { std::cout << "world\n"; co_return; }, foxy::detached);

        std::cout << "waiting for child coroutines to complete\n";
      },
      foxy::detached);

    auto threads = std::vector<std::thread>();
    threads.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
      threads.emplace_back(
        [&](void) -> void
        {
          io.run();
        });
    }

    for (auto& t : threads) {
      t.join();
    }
  }
}