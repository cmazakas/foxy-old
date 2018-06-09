#ifndef FOXY_PMR_FIELDS_HPP_
#define FOXY_PMR_FIELDS_HPP_

#include <boost/beast/http/fields.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace foxy
{
namespace pmr
{

using basic_fields =
  boost::beast::http::basic_fields<
    boost::container::pmr::polymorphic_allocator<char>
  >;

} // pmr
} // foxy

#endif // FOXY_PMR_FIELDS_HPP_