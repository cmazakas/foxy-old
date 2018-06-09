#ifndef FOXY_PMR_FLAT_BUFFER_HPP_
#define FOXY_PMR_FLAT_BUFFER_HPP_

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace foxy
{
namespace pmr
{

using basic_flat_buffer = boost::beast::basic_flat_buffer<
  boost::container::pmr::polymorphic_allocator<char>
>;

} // pmr
} // foxy

#endif // FOXY_PMR_FLAT_BUFFER_HPP_