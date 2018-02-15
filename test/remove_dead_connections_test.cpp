#include "foxy/remove_dead_connections.hpp"

#include <list>
#include <chrono>
#include <cstddef>
#include <algorithm>

#include <catch.hpp>

namespace
{
struct connection
{
  using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;

  bool            io_pending_;
  time_point_type last_activity_;

  auto set_io_pending(void) noexcept -> void
  {
    io_pending_ = true;
  }

  auto is_io_pending(void) const noexcept -> bool
  {
    return io_pending_;
  }

  auto get_last_activity(void) const noexcept -> time_point_type
  {
    return last_activity_;
  }

  auto update_last_activity(void) noexcept -> void
  {
    last_activity_ = std::chrono::steady_clock::now();
  }
};
}

TEST_CASE("Removing dead connections")
{
  SECTION("should not remove any non-I/O pending ones")
  {
    auto const num_conns = std::size_t{5};
    auto conns = std::list<connection>(num_conns);

    auto const end = foxy::remove_dead_connections(
      conns.begin(), conns.end(),
      std::chrono::seconds(0));

    REQUIRE(std::distance(conns.begin(), end) == num_conns);
  }

  SECTION("should remove an expired connection")
  {
    auto conn = connection();

    auto conns = std::list<connection>();
    conns.push_back(conn);

    for (auto& c : conns) {
      c.set_io_pending();
    }

    auto const end = foxy::remove_dead_connections(
      conns.begin(), conns.end(),
      std::chrono::seconds(0));

    auto const num_conns = std::distance(conns.begin(), end);

    REQUIRE(num_conns == 0);
  }
}