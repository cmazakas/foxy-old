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
#include "foxy/header_parser.hpp"
#include "foxy/async_read_body.hpp"

namespace foxy
{
struct connection
  : public boost::asio::coroutine
  , public std::enable_shared_from_this<connection>
{
public:
  using socket_type = boost::asio::ip::tcp::socket;
  using buffer_type = boost::beast::flat_buffer;
  using strand_type =
    boost::asio::strand<boost::asio::io_context::executor_type>;

  using header_parser_type = header_parser<>;

private:
  socket_type socket_;
  strand_type strand_;
  buffer_type buffer_;

  header_parser_type parser_;

  template <typename F>
  auto make_stranded(F&& f)
  {
    return boost::asio::bind_executor(strand_, std::forward<F>(f));
  }

public:
  connection(socket_type socket)
    : socket_(std::move(socket))
    , strand_(socket_.get_executor())
  {
  }

  auto socket(void) & noexcept -> socket_type&
  {
    return socket_;
  }

  auto buffer(void) & noexcept -> buffer_type&
  {
    return buffer_;
  }

  auto executor(void) & noexcept -> strand_type&
  {
    return strand_;
  }

  auto run(
    boost::system::error_code const  ec = {},
    std::size_t const bytes_transferred = 0) -> void;
};

#include <boost/asio/yield.hpp>
auto foxy::connection::run(
  boost::system::error_code const ec,
  std::size_t const bytes_transferred) -> void
{
  namespace asio   = boost::asio;
  namespace http   = boost::beast::http;

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

    // yield {
    //   auto found_match = false;

    //   fusion::for_each(
    //     routes_,
    //     [=, &found_match](auto const& route) -> void
    //     {
    //       if (found_match) { return; }

    //       using rule_type = decltype(route.rule);
    //       using sig_type  = typename rule_type::sig_type;
    //       using synth_attr_type = ct::return_type_t<sig_type>;

    //       if (parse_attrs_into_handler<synth_attr_type>(route)) {
    //         found_match = true;
    //       }
    //     });
    // }

    socket_.shutdown(socket_type::shutdown_both);
  }
}
#include <boost/asio/unyield.hpp>

} // foxy

#endif // FOXY_CONNECTION_HPP_