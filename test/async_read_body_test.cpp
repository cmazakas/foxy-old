#include "foxy/async_read_body.hpp"

#include <string>
#include <sstream>
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
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include "foxy/test/stream.hpp"

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

    auto req_parser = http::request_parser<http::empty_body>{};

    auto const bytes_read = http::read_header(stream, buf, req_parser);
    REQUIRE(bytes_read > 0);

    auto const target = req_parser.get().target();
    REQUIRE(target == "/rawr");

    REQUIRE(stream.buffer().size() > 0);

    auto string_parser = http::request_parser<http::string_body>{std::move(req_parser)};
    http::read(stream, buf, string_parser);

    auto msg = string_parser.release();

    // auto fut = foxy::async_read_body<http::string_body>(
    //   stream, buf, std::move(req_parser), asio::use_future);

    // ioc.run();

    // auto msg = fut.get();
    REQUIRE(msg.body() == "I bestow the heads of virgins and the first-born sons.");
  }
}