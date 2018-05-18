#include "foxy/handlers/not_found.hpp"

#include <boost/spirit/include/qi_raw.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_plus.hpp>

#include "foxy/route.hpp"
#include "foxy/parse_string_view.hpp"

namespace foxy
{
namespace handlers
{

auto not_found_handler(
  boost::system::error_code const ec,
  std::shared_ptr<foxy::session>  session,
  foxy::string_view const         target)
-> foxy::awaitable<
  void,
  boost::asio::strand<boost::asio::io_context::executor_type>>
{
  co_return;
}

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
>
{
  namespace qi = boost::spirit::qi;

  using iterator_type = foxy::string_view::iterator;

  auto not_found_rule =
    qi::rule<iterator_type, foxy::string_view()>(
      qi::raw[+qi::char_]);

  auto const f = std::function<foxy::awaitable<
    void,
    boost::asio::strand<boost::asio::io_context::executor_type>
  >(
    boost::system::error_code const ec,
    std::shared_ptr<foxy::session>  session,
    foxy::string_view const         target)
  >(not_found_handler);

  return { not_found_rule, f };
}

} // handlers
} // foxy