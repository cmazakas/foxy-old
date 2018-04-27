#ifndef FOXY_COROUTINE_HPP_
#define FOXY_COROUTINE_HPP_

#ifndef BOOST_ASIO_HAS_CO_AWAIT
#define BOOST_ASIO_HAS_CO_AWAIT
#endif

#include <boost/asio/experimental/co_spawn.hpp>
#include <boost/asio/experimental/detached.hpp>
#include <boost/asio/experimental/redirect_error.hpp>

namespace foxy
{
using boost::asio::experimental::co_spawn;
using boost::asio::experimental::detached;
using boost::asio::experimental::awaitable;
using boost::asio::experimental::redirect_error_t;

namespace this_coro = boost::asio::experimental::this_coro;

template <typename CompletionToken>
auto make_redirect_error_token(
  CompletionToken&&          token,
  boost::system::error_code& ec)
{
  return redirect_error_t<CompletionToken>(
    std::forward<CompletionToken>(token), ec);
}
}

#endif // FOXY_COROUTINE_HPP_