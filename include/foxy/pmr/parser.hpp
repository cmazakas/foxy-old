#ifndef FOXY_PMR_PARSER_HPP_
#define FOXY_PMR_PARSER_HPP_

#include <boost/beast/http/parser.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace foxy
{
namespace pmr
{

template <
  bool     isRequest,
  typename Body
>
using parser = boost::beast::http::parser<
  isRequest, Body, boost::container::pmr::polymorphic_allocator<char>>;

template <typename Body>
using request_parser = boost::beast::http::request_parser<
  Body, boost::container::pmr::polymorphic_allocator<char>>;

template <typename Body>
using response_parser = boost::beast::http::response_parser<
  Body, boost::container::pmr::polymorphic_allocator<char>>;

} // pmr
} // foxy

#endif // FOXY_PMR_PARSER_HPP_