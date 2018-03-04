#ifndef FOXY_DETAIL_STOP_IO_CONTEXT_HPP_
#define FOXY_DETAIL_STOP_IO_CONTEXT_HPP_

namespace foxy
{
namespace detail
{
auto stop_io_context() -> void
{

}
} // detail
} // foxy

#endif // FOXY_DETAIL_STOP_IO_CONTEXT_HPP_

/*
class io_threads
{
	std::vector<std::thread> threads_;
  	boost::asio::io_context  io_;

  	std::mutex m_;
  	std::condition_variable cv_;

  	std::function<void(void)> f_;

  	std::atomic<unsigned> running_threads_ = 0;

  	bool is_destructing_ = false;

  	void
    thread_task()
    {
      while (true) {
        ++running_threads_;
        io_.run();
        if (--running_threads_ > 0 || !f_) {
          std::unique_lock<std::mutex> lock(m_);

          cv_.wait(
            lock,
            [self = this]() -> bool
            {
              return is_destructing_ || !io_.stopped();
            });

          if (is_destructing_) { break; }
        } else {
          f_();
          f_ = {};
          io_.restart();
        }
      }
    }

public:
  	/// Create the thread pool with `n` threads
	explicit
    io_threads(std::size_t n)
      : threads_(n, [self=this]() { self->thread_task(); })
      {
      };

  	~io_threads()
    {
      io_.stop();

      {
        std::lock_guard<std::mutex> lock(m_);
        is_destructing_ = true;
        cv_.notify_all();
      }

      for (auto& t : threads_) { t.join(); }
    }

  	boost::asio::io_context&
    context() &
    {
      return io_;
    }

  	// Stop the threads, execute `f` on a thread, and resume
  	// thread-safety: none
  	// should only be called again once callback completes
  	template<class Function>
    void
    async_run_exclusive(Function const& f)
    {
      f_ = f;
      io_.stop();
    }
};

*/