#ifndef FOXY_CLIENT_HPP_
#define FOXY_CLIENT_HPP_

#include <string>

#include <boost/system/error_code.hpp>

#include <boost/asio/async_result.hpp>
#include <boost/asio/experimental/co_spawn.hpp>
#include <boost/asio/experimental/detached.hpp>

#include <boost/beast/http/message.hpp>

namespace foxy
{
template <typename CompletionToken>
auto send_request(
  std::string const& host,
  std::string const& port,
  std::string const& target,
  CompletionToken&&  token)
{
  using boost::system::error_code;

  namespace asio = boost::asio;

  asio::async_completion<
    CompletionToken, void(error_code)> init(token);

  return init.result.get();
}
} // foxy

#endif // FOXY_CLIENT_HPP_