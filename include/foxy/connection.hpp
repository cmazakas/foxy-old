#ifndef FOXY_CONNECTION_HPP_
#define FOXY_CONNECTION_HPP_

#include <memory>      // shared_ptr, make_shared, enable_shared_from_this
#include <cstddef>     // size_t
#include <utility>     // forward/move
#include <iostream>    // cout debugging
#include <type_traits> // remove_reference, decay

#include <boost/callable_traits.hpp>

#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>

#include <boost/beast/core/string.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#include <boost/fusion/container/list.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include <boost/spirit/include/qi_parse.hpp>

#include "foxy/log.hpp"
#include "foxy/async_read_body.hpp"

namespace foxy
{
template <typename RouteList>
struct connection
  : public boost::asio::coroutine
  , std::enable_shared_from_this<connection<RouteList>>
{
public:
  using socket_type = boost::asio::ip::tcp::socket;
  using buffer_type = boost::beast::flat_buffer;
  using strand_type =
    boost::asio::strand<boost::asio::io_context::executor_type>;

  using header_parser_type =
    boost::beast::http::request_parser<boost::beast::http::empty_body>;

private:
  socket_type socket_;
  strand_type strand_;
  buffer_type buffer_;

  header_parser_type parser_;

  RouteList const& routes_;

  template <typename F>
  auto make_stranded(F&& f)
  {
    return boost::asio::bind_executor(strand_, std::forward<F>(f));
  }

  template <
    typename SynthAttrType,
    typename Route,
    std::enable_if_t<
      std::is_same<void, SynthAttrType>::value, bool> = false
  >
  auto parse_attrs_into_handler(Route const& route) -> bool
  {
    using boost::system::error_code;

    namespace qi   = boost::spirit::qi;
    namespace http = boost::beast::http;
    namespace asio = boost::asio;

    auto const& rule    = route.rule;
    auto const& handler = route.handler;
    auto const  target  = parser_.get().target();

    if (
      qi::parse(target.begin(), target.end(), rule))
    {
      asio::post(
        make_stranded(
          [
            self = this->shared_from_this(),
            &handler
          ](void)
          {
            handler({}, self->parser_, std::move(self));
          }));

      return true;
    }

    return false;
  }

  template <
    typename SynthAttrType,
    typename Route,
    std::enable_if_t<
      !std::is_move_constructible<SynthAttrType>::value &&
      !std::is_same<void, SynthAttrType>::value, bool> = false
  >
  auto parse_attrs_into_handler(Route const& route) -> bool
  {
    using boost::system::error_code;

    namespace qi   = boost::spirit::qi;
    namespace http = boost::beast::http;
    namespace asio = boost::asio;

    // requires default-constructible for all parsing types
    SynthAttrType attr{};

    auto const& rule    = route.rule;
    auto const& handler = route.handler;
    auto const  target  = parser_.get().target();

    if (
      qi::parse(target.begin(), target.end(), rule, attr))
    {
      asio::post(
        make_stranded(
          [
            self = this->shared_from_this(),
            &handler,
            attr
          ](void)
          {
            handler({}, self->parser_, std::move(self), attr);
          }));

      return true;
    }

    return false;
  }

  template <
    typename SynthAttrType,
    typename Route,
    std::enable_if_t<
      std::is_move_constructible<SynthAttrType>::value, bool> = false
  >
  auto parse_attrs_into_handler(Route const& route) -> bool
  {
    using boost::system::error_code;

    namespace qi   = boost::spirit::qi;
    namespace http = boost::beast::http;
    namespace asio = boost::asio;

    // requires default-constructible for all parsing types
    SynthAttrType attr{};

    auto const& rule    = route.rule;
    auto const& handler = route.handler;
    auto const  target  = parser_.get().target();

    if (
      qi::parse(target.begin(), target.end(), rule, attr))
    {
      asio::post(
        make_stranded(
          [
            self = this->shared_from_this(),
            &handler,
            attr_v = std::move(attr)
          ](void)
          {
            handler({}, self->parser_, std::move(self), std::move(attr_v));
          }));

      return true;
    }

    return false;
  }

public:
  connection(socket_type socket, RouteList const& routes)
    : socket_{std::move(socket)}
    , strand_{socket_.get_executor()}
    , routes_{routes}
  {
  }

  auto get_socket(void) & noexcept -> socket_type&
  {
    return socket_;
  }

  auto get_buffer(void) & noexcept -> buffer_type&
  {
    return buffer_;
  }

  auto run(
    boost::system::error_code const  ec = {},
    std::size_t const bytes_transferred = 0,
    std::shared_ptr<header_parser_type> header_parser = nullptr) -> void;
};

#include <boost/asio/yield.hpp>
template <typename RouteList>
auto foxy::connection<RouteList>::run(
  boost::system::error_code const ec,
  std::size_t const bytes_transferred,
  std::shared_ptr<header_parser_type> header_parser) -> void
{
  namespace ct     = boost::callable_traits;
  namespace qi     = boost::spirit::qi;
  namespace asio   = boost::asio;
  namespace http   = boost::beast::http;
  namespace fusion = boost::fusion;

  using boost::system::error_code;

  reenter (*this) {
    yield {
      http::async_read_header(
        socket_, buffer_, parser_,
        make_stranded(
          [self = this->shared_from_this()](
            error_code  const ec,
            std::size_t const bytes_transferred) -> void
          {
            if (ec) {
              return fail(ec, "header parsing");
            }
            self->run({}, bytes_transferred);
          }));
    }

    yield {
      auto found_match = false;

      fusion::for_each(
        routes_,
        [=, &found_match](auto const& route) -> void
        {
          if (found_match) { return; }

          using rule_type = decltype(route.rule);
          using sig_type  = typename rule_type::sig_type;
          using synth_attr_type = ct::return_type_t<sig_type>;

          if (parse_attrs_into_handler<synth_attr_type>(route)) {
            found_match = true;
          }
        });
    }

    socket_.shutdown(socket_type::shutdown_both);
  }
}
#include <boost/asio/unyield.hpp>

} // foxy

#endif // FOXY_CONNECTION_HPP_