#include "foxy/async_read_body.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/beast/core/string.hpp>
#include <boost/beast/core/ostream.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/vector_body.hpp>

#include "foxy/test/stream.hpp"
#include "foxy/header_parser.hpp"

#include <catch.hpp>

TEST_CASE("async_read_body")
{
  namespace beast = boost::beast;
  namespace asio  = boost::asio;
  namespace http  = boost::beast::http;
  namespace test  = boost::beast::test;

  using boost::system::error_code;

  SECTION("should do as advertised")
  {
    asio::io_context ioc;

    auto stream = test::stream(ioc);
    auto buf    = beast::flat_buffer();
    auto timer  = asio::steady_timer(ioc);

    auto req = http::request<http::string_body>(http::verb::get, "/rawr", 11);
    req.body() = "I bestow the heads of virgins and the first-born sons.";
    req.prepare_payload();

    REQUIRE(req.payload_size().get() > 0);

    beast::ostream(stream.buffer()) << req;

    foxy::header_parser<> header_parser;

    auto const bytes_read = http::read_header(stream, buf, header_parser);
    REQUIRE(bytes_read > 0);

    auto const target = header_parser.get().target();
    REQUIRE(target == "/rawr");

    auto fut = foxy::async_read_body<http::string_body>(
      stream, buf, timer,
      std::move(header_parser),
      asio::use_future);

    ioc.run();

    auto msg = http::request<http::string_body>(fut.get());
    REQUIRE(msg.body() == "I bestow the heads of virgins and the first-born sons.");
  }

  SECTION("should be able to handle body sizes in the MB range")
  {
    using body_t = http::vector_body<std::uint8_t>;

    asio::io_context io;

    auto stream = test::stream(io);
    auto buf    = beast::flat_buffer();
    auto timer  = asio::steady_timer(io);

    constexpr
    auto const body_size = std::uint64_t{4096 * 1024};

    {
      auto req = http::request<body_t>(http::verb::post, "/", 11);
      req.body() = std::vector<std::uint8_t>(body_size, 137);
      req.prepare_payload();
      beast::ostream(stream.buffer()) << req;
    }

    foxy::header_parser<> header_parser;
    header_parser.body_limit(body_size);

    http::read_header(stream, buf, header_parser);

    auto fut = foxy::async_read_body<body_t>(
      stream, buf, timer,
      std::move(header_parser),
      asio::use_future);

    io.run();

    auto req = http::request<body_t>(fut.get());

    REQUIRE(req.body().size() == body_size);
  }

  SECTION("should handle stream interruptions")
  {
    using body_t = http::vector_body<std::uint8_t>;

    asio::io_context io;

    // fail after 300 reads
    auto fc     = test::fail_counter(300);
    auto stream = test::stream(io, fc);
    auto buf    = beast::flat_buffer();
    auto timer  = asio::steady_timer(io);

    constexpr
    auto const body_size = std::uint64_t{4096 * 1024};

    {
      auto req = http::request<body_t>(http::verb::post, "/", 11);
      req.body() = std::vector<std::uint8_t>(body_size, 137);
      req.prepare_payload();
      beast::ostream(stream.buffer()) << req;
    }

    foxy::header_parser<> header_parser;
    header_parser.body_limit(body_size);

    http::read_header(stream, buf, header_parser);

    auto fut = foxy::async_read_body<body_t>(
      stream, buf, timer,
      std::move(header_parser),
      asio::use_future);

    io.run();

    REQUIRE_THROWS(fut.get());
  }
}