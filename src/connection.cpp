#include "foxy/connection.hpp"

namespace foxy
{
connection::connection(socket_type socket)
  : socket_(std::move(socket))
  , strand_(socket_.get_executor())
{
}

auto connection::socket(void) & noexcept -> socket_type&
{
  return socket_;
}

auto connection::buffer(void) & noexcept -> buffer_type&
{
  return buffer_;
}

auto connection::executor(void) & noexcept -> strand_type&
{
  return strand_;
}

#include <boost/asio/yield.hpp>
auto connection::run(
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