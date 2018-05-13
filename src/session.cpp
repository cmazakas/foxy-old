#include "foxy/session.hpp"

foxy::session::session(foxy::session::stream_type stream)
: stream_(std::move(stream))
, strand_(stream_.get_executor())
, timer_(stream_.get_executor().context())
{
}

auto foxy::session::timeout() -> foxy::awaitable<void, strand_type>
{
  co_return;
}

auto foxy::session::buffer() & noexcept -> buffer_type&
{
  return buffer_;
}

auto foxy::session::stream() & noexcept -> stream_type&
{
  return stream_;
}

auto foxy::session::parser() & noexcept -> parser_type&
{
  return parser_;
}

auto foxy::session::timer() & noexcept -> timer_type&
{
  return timer_;
}