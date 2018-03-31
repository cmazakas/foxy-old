#include "foxy/listener.hpp"

#include <utility>
#include <chrono>

#include <boost/asio/post.hpp>
#include <boost/asio/associated_executor.hpp>

#include "foxy/log.hpp"

namespace foxy
{

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

      std::make_shared<connection>(
        std::move(socket_), handler_)->run();
    }
  }
}
#include <boost/asio/unyield.hpp>

} // foxy