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

template <typename T>
struct polymorphic_wrapper
{
public:
  using allocator_type = boost::container::pmr::polymorphic_allocator<T>;

public:
  allocator_type allocator_;

public:
  using value_type = T;

  polymorphic_wrapper() noexcept = default;

  polymorphic_wrapper(allocator_type const& other_allocator) noexcept
    : allocator_(other_allocator)
  {

  }

  polymorphic_wrapper(boost::container::pmr::memory_resource* resource)
  {
    allocator_ = allocator_type(resource);
  }

  polymorphic_wrapper(polymorphic_wrapper const& other)
  {
    allocator_ = other.allocator_;
  }

  template<typename U>
  polymorphic_wrapper(polymorphic_wrapper<U> const& other) noexcept
  {
    allocator_ = other.allocator_;
  }

  auto operator=(polymorphic_wrapper const& other) noexcept
    -> polymorphic_wrapper&
  {
    return *this;
  }

  auto allocate(std::size_t const size) -> T*
  {
    return allocator_.allocate(size);
  }

  auto deallocate(T* t, std::size_t const size) -> void
  {
    return allocator_.deallocate(t, size);
  }

  template <typename U, class... Args>
  auto construct(U* u, Args&&... args) -> void
  {
    allocator_.construct(u, std::forward<Args>(args)...);
  }

  template<typename U>
  auto destroy(U* u) -> void
  {
    allocator_.destroy(u);
  }

  auto select_on_container_copy_construction() const -> polymorphic_wrapper
  {
    return allocator_.select_on_container_copy_construction();
  }

  auto resource() const -> boost::container::pmr::memory_resource*
  {
    return allocator_.resource();
  }
};

template <typename T1, typename T2>
auto operator==(
  polymorphic_wrapper<T1> const& pw1,
  polymorphic_wrapper<T2> const& pw2) -> bool
{
  return pw1.allocator_ == pw2.allocator_;
}

template <typename T1, typename T2>
auto operator!=(
  polymorphic_wrapper<T1> const& pw1,
  polymorphic_wrapper<T2> const& pw2) -> bool
{
  return pw1.allocator_ != pw2.allocator_;
}

auto make_req(
  asio::io_context& io
) -> foxy::awaitable<void, asio::io_context::executor_type>
{
  auto token = co_await foxy::this_coro::token();

  auto const* const host    = "www.google.com";
  auto const* const port    = "80";
  auto const        version = 11; // HTTP/1.1

  using allocator_type = polymorphic_wrapper<char>;
  using fields_type    = http::basic_fields<allocator_type>;

  // using body_type =
  //   http::basic_string_body<char, std::char_traits<char>, allocator_type>;

  using body_type = http::string_body;

  boost::container::pmr::unsynchronized_pool_resource pool;

  auto allocator = polymorphic_wrapper<char>(&pool);

  auto fields  = fields_type(allocator);
  // auto body    = body_type(allocator_type(&allocator));

  auto request = http::request<http::empty_body>(
    http::verb::get, "/", version);

  auto response = http::response<body_type, fields_type>(
    std::piecewise_construct,
    std::make_tuple(""),
    std::make_tuple(&pool)
  );

  co_await foxy::async_send_request(
    io,
    host, port,
    std::move(request),
    response,
    token);

  REQUIRE(response.result_int() == 200);
  REQUIRE(response.body().size() > 0);
}

}


TEST_CASE("Our HTTP client")
{
  SECTION("should be able to callout to google")
  {
    asio::io_context io;
    foxy::co_spawn(io, [&]() { return make_req(io); }, foxy::detached);
    io.run();
  }
}