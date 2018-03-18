#include "foxy/listener.hpp"

#include <utility>
#include <chrono>

#include <boost/asio/post.hpp>
#include <boost/asio/associated_executor.hpp>

#include "foxy/log.hpp"

namespace foxy
{

listener::listener(
  boost::asio::io_context&              io,
  boost::asio::ip::tcp::endpoint const& endpoint)
: acceptor_(io, endpoint)
, socket_(io)
{
}

#include <boost/asio/yield.hpp>
auto listener::accept(boost::system::error_code const ec) -> void
{
  reenter(accept_coro_)
  {
    for (;;) {
      yield acceptor_.async_accept(
        socket_,
        [self = this->shared_from_this()]
        (boost::system::error_code const& ec) -> void
        {
          self->accept(ec);
        });

      if (ec) {
        fail(ec, "accepting new connection");
        continue;
      }

      auto conn = std::make_shared<connection>(std::move(socket_));
      conn->on_request(this->handler_);

      auto const cb = [conn](void) -> void
      {
        conn->run();
      };

      conn->executor().post(cb, boost::asio::get_associated_allocator(cb));
    }
  }
}
#include <boost/asio/unyield.hpp>

} // foxy