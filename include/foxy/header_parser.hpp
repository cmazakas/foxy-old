#ifndef FOXY_HEADER_PARSER_
#define FOXY_HEADER_PARSER_

#include <memory>

#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>

namespace foxy
{
template <typename Allocator = std::allocator<char>>
using header_parser =
  boost::beast::http::request_parser<
    boost::beast::http::empty_body, Allocator>;
}

#endif // FOXY_HEADER_PARSER_