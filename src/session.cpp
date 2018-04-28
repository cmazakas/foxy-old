#include "foxy/session.hpp"

namespace asio = boost::asio;

namespace foxy
{

session::session(socket_type socket)
: socket_(std::move(socket))
, strand_(socket.get_executor())
{
}

auto session::start(void) -> void
{
  co_spawn(
    strand_,
    [self = this->shared_from_this()]()
    {
      return self->request_handler();
    },
    detached);

  co_spawn(
    strand_,
    [self = this->shared_from_this()]()
    {
      return self->timeout();
    },
    detached);
}

auto session::request_handler(void) -> awaitable<void, strand_type>
{
  co_return;
}

auto session::timeout(void) -> awaitable<void, strand_type>
{
  co_return;
}

} // foxy