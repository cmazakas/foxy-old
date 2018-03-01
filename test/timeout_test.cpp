#include "foxy/timeout_connections.hpp"

#include <list>
#include <cstddef>
#include <algorithm>

#include <catch.hpp>

namespace
{
struct connection
{
  using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;

  bool            io_pending_    = false;
  bool            is_open_       = true;
  time_point_type last_activity_ = std::chrono::steady_clock::now();

  auto io_pending(void) const noexcept -> bool
  {
    return io_pending_;
  }

  auto io_pending(bool const iop) noexcept -> void
  {
    io_pending_ = iop;
  }

  auto last_activity(void) const noexcept -> time_point_type
  {
    return last_activity_;
  }

  auto close(void)
  {
    is_open_ = false;
  }
};
}

TEST_CASE("Our timeout handler")
{
  SECTION("should not close any non-I/O pending operations")
  {
    auto const num_conns = std::size_t{5};

    auto conns = std::list<connection>(num_conns);

    REQUIRE(
      std::all_of(
        conns.begin(), conns.end(),
        [](connection const& conn) -> bool
        {
          return conn.is_open_ && !conn.io_pending();
        }));

    foxy::timeout_connections(
      conns.begin(), conns.end(),
      std::chrono::seconds(0));

    REQUIRE(
      std::all_of(
        conns.begin(), conns.end(),
        [](connection const& conn) -> bool
        {
          return conn.is_open_ && !conn.io_pending();
        }));
  }

  SECTION("should close all open and expired connections")
  {
    auto const num_conns = std::size_t{5};

    auto conns = std::list<connection>(num_conns);

    for (auto& conn : conns) {
      conn.io_pending(true);
    }

    foxy::timeout_connections(
      conns.begin(), conns.end(),
      std::chrono::seconds(0));

    REQUIRE(
      std::all_of(
        conns.begin(), conns.end(),
        [](connection const& conn) -> bool
        {
          return !conn.is_open_;
        }));
  }

  SECTION("should not close I/O operations with recent activity")
  {
    auto const num_conns = std::size_t{5};

    auto conns = std::list<connection>(num_conns);

    for (auto& conn : conns) {
      conn.io_pending(true);
    }

    foxy::timeout_connections(
      conns.begin(), conns.end(),
      std::chrono::seconds(30));

    REQUIRE(
      std::all_of(
        conns.begin(), conns.end(),
        [](connection const& conn) -> bool
        {
          return conn.is_open_;
        }));
  }
}