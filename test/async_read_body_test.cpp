#include "foxy/async_read_body.hpp"

#include <string>
#include <iostream>
#include <sstream>

#include <boost/asio/io_context.hpp>

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

    auto buf = beast::flat_buffer{};

    auto req = http::request<http::string_body>{http::verb::get, "/rawr", 11};
    req.body() = "I bestow the heads of virgins and the first-born sons.";

    beast::ostream(stream.buffer()) << req;

    auto req_parser = http::request_parser<http::string_body>{};

    auto const bytes_read = http::read_header(stream, buf, req_parser);
    REQUIRE(bytes_read > 0);

    // foxy::async_read_body(
    //   AsyncReadStream&                    stream,
    //   DynamicBuffer&                      buffer,
    //   boost::beast::http::request_parser<
    //     InBody, Allocator>&&              parser,
    //   MessageHandler&&                    handler);
  }
}