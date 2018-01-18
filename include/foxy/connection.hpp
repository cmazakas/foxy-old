#ifndef FOXY_CONNECTION_HPP_
#define FOXY_CONNECTION_HPP_

#include <memory>
#include <cstddef>
#include <iostream>
#include <type_traits>

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

#include <boost/mpl/begin_end.hpp>

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
  using strand_type = boost::asio::strand<
    boost::asio::io_context::executor_type>;
  using buffer_type = boost::beast::flat_buffer;

private:
  socket_type socket_;
  strand_type strand_;
  buffer_type buffer_;

  RouteList const& routes_;

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

  auto run(
    boost::system::error_code const  ec = {},
    std::size_t const bytes_transferred = 0,
    std::shared_ptr<
      boost::beast::http::request_parser<
        boost::beast::http::empty_body>>  header_parser = nullptr) -> void;
};

#include <boost/asio/yield.hpp>
template <typename RouteList>
auto foxy::connection<RouteList>::run(
  boost::system::error_code const ec,
  std::size_t const bytes_transferred,
  std::shared_ptr<
    boost::beast::http::request_parser<
      boost::beast::http::empty_body>>  header_parser) -> void
{
  namespace qi     = boost::spirit::qi;
  namespace mpl    = boost::mpl;
  namespace asio   = boost::asio;
  namespace http   = boost::beast::http;
  namespace fusion = boost::fusion;

  using boost::system::error_code;

  reenter (*this) {
    yield {
      auto tmp_parser =
        std::make_shared<http::request_parser<http::empty_body>>();

      auto& p = *tmp_parser;

      http::async_read_header(
        socket_, buffer_, p,
        asio::bind_executor(
          strand_,
          [
            self   = this->shared_from_this(),
            parser = std::move(tmp_parser)
          ](
            error_code  const ec,
            std::size_t const bytes_transferred
          ) -> void
          {
            self->run(ec, bytes_transferred, std::move(parser));
          }));
    }

    yield {
      auto const partial_request =
        http::request<http::empty_body>{header_parser->get()};

      auto const target = partial_request.target();

      using element_type =
        std::pointer_traits<
          std::decay_t<decltype(header_parser)>>::element_type;

      auto self = this;

      fusion::for_each(
        routes_,
        [=, &header_parser](auto const& route) -> void
        {
          auto const& rule    = route.rule;
          auto const& handler = route.handler;

          using rule_type  = std::decay_t<decltype(rule)>;
          using route_type = std::decay_t<decltype(route)>;
          using body_type  = typename route_type::body_type;

          using sig_type = typename rule_type::sig_type;
          // using synth_attribute_type = typename mpl::begin<sig_type>::type;
          using synth_attribute_type = decltype(std::declval<sig_type>()());

          synth_attribute_type val;

          if (
            qi::parse(target.begin(), target.end(), rule, val)
          ) {
            foxy::async_read_body<body_type>(
              self->socket_, self->buffer_,
              std::move(*header_parser),
              asio::bind_executor(
                self->strand_,
                [sp = self->shared_from_this(), &handler, val]
                (error_code ec, http::request<body_type>&& req)
                {
                  handler(ec, std::move(req), std::move(sp), val);
                }));
          }
        });
    }

    socket_.shutdown(socket_type::shutdown_both);
  }
}
#include <boost/asio/unyield.hpp>

} // foxy

#endif // FOXY_CONNECTION_HPP_