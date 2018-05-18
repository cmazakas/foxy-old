#ifndef FOXY_HANDLERS_NOT_FOUND_HPP_
#define FOXY_HANDLERS_NOT_FOUND_HPP_

#include <memory>
#include <functional>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/system/error_code.hpp>

#include "foxy/route.hpp"
#include "foxy/session.hpp"
#include "foxy/coroutine.hpp"
#include "foxy/string_view.hpp"

namespace foxy
{
namespace handlers
{

auto not_found_handler(
  boost::system::error_code const ec,
  std::shared_ptr<foxy::session>  session,
  string_view const               target)
-> foxy::awaitable<
  void,
  boost::asio::strand<boost::asio::io_context::executor_type>>;

auto not_found() -> foxy::route<
  foxy::string_view::iterator, foxy::string_view, boost::spirit::unused_type, boost::spirit::unused_type,
  std::function<foxy::awaitable<
    void,
    boost::asio::strand<boost::asio::io_context::executor_type>
  >(
    boost::system::error_code const,
    std::shared_ptr<foxy::session>,
    foxy::string_view const)
  >
>;
} // handlers
} // foxy

#endif // FOXY_HANDLERS_NOT_FOUND_HPP_