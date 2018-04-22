#ifndef FOXY_ASYNC_RESULT_HPP_
#define FOXY_ASYNC_RESULT_HPP_

#include <boost/asio/async_result.hpp>

namespace foxy
{
template <
  typename CompletionToken,
  typename Signature
>
using init_fn_result_type = BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, Signature);

template <
  typename CompletionToken,
  typename Signature
>
using init_fn_handler_type = BOOST_ASIO_HANDLER_TYPE(CompletionToken, Signature);
}

#endif // FOXY_ASYNC_RESULT_HPP_