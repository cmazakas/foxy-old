#include "foxy/async_read_body.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

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

  SECTION("should do as advertised")
  {
    asio::io_context ioc{};

    auto stream = test::stream{ioc};
    auto buf    = beast::flat_buffer{};

    auto req = http::request<http::string_body>{http::verb::get, "/rawr", 11};
    req.body() = "I bestow the heads of virgins and the first-born sons.";
    req.prepare_payload();

    REQUIRE(req.payload_size().get() > 0);

    beast::ostream(stream.buffer()) << req;

    auto header_parser = foxy::header_parser<>{};

    auto const bytes_read = http::read_header(stream, buf, header_parser);
    REQUIRE(bytes_read > 0);

    auto const target = header_parser.get().target();
    REQUIRE(target == "/rawr");

    boost::asio::steady_timer timer{ioc};

    auto fut = foxy::async_read_body<http::string_body>(
      stream, buf,
      std::move(header_parser),
      timer,
      asio::use_future);

    ioc.run();

    auto msg = http::request<http::string_body>{fut.get()};
    REQUIRE(msg.body() == "I bestow the heads of virgins and the first-born sons.");
  }

  SECTION("should be able to handle body sizes in the MB range")
  {
    using body_t = http::vector_body<std::uint8_t>;

    asio::io_context io{};

    auto stream = test::stream{io};
    auto buf    = beast::flat_buffer{};

    {
      auto req = http::request<body_t>{http::verb::post, "/", 11};
      req.body() = std::vector<std::uint8_t>(4096, 137);
      req.prepare_payload();
      beast::ostream(stream.buffer()) << req;
    }

    auto header_parser = foxy::header_parser<>{};
    http::read_header(stream, buf, header_parser);

    asio::steady_timer timer{io};

    auto fut = foxy::async_read_body<body_t>(
    stream, buf,
    std::move(header_parser),
    timer,
    asio::use_future);

    io.run();

    auto req = http::request<body_t>{fut.get()};

    REQUIRE(req.body().size() == 4096);
  }
}