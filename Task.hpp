/*
 * C++11 introduced concurrency features like std::async, future and promise,
 * and while these tools are often sufficient and useful, they can sometimes
 * behave in unexpected ways.
 *
 * Specifically, a std::future returned from std::async with the 'async'
 * launch policy will block (i.e., join the underlying thread) when its
 * destructor is called. This is often undesirable. The 'Task' type in this
 * header has two advantages over std::{shared_}future / std::async
 *
 * 1. It will always run the task asynchronously, no launch policy is necessary
 * 2. The underlying thread is detached, so no blocking will occur unless the
 * user calls 'get' explicitly.
 *
 * Note: Constructing a task does not guarantee that the underlying thread
 * will run to completion if main terminates before the task is finished.
 * To enable this behavior (if the system is using POSIX threads), the user
 * may perform a call to "pthread_exit", before the return from main. Doing
 * so will cause the main thread to block at that point, until all threads
 * have completed.
 *
 * For more information, see:
 * http://man7.org/linux/man-pages/man3/pthread_exit.3.html
 */

#include <future>

template <typename R>
class Task {
    std::shared_future<R> future;

public:
    template <typename F, typename... Args>
    Task(F&& func, Args&&... args)
        : future{} {
        std::packaged_task<R(Args...)> package{func};
        future = package.get_future().share();

        std::thread(std::move(package), std::forward(args)...).detach();
    }

    bool valid() const { return future.valid(); }

    // If R == void, a reference cannot be formed, so use this helper, which
    // does nothing in that case.
    const typename std::add_lvalue_reference<R>::type get() const {
        return future.get();
    }
    void wait() const { future.wait(); }

    template <class Rep, class Period>
    std::future_status
    wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const {
        return future.wait_for(timeout_duration);
    }

    template <class Clock, class Duration>
    std::future_status wait_until(
        const std::chrono::time_point<Clock, Duration>& timeout_time) const {
        return future.wait_for(timeout_time);
    }
};

/**
 * 'run' is analogous to 'std::async'. It returns a 'Task' for the given
 * functor. Note that this task is asynchronous and will start immediately.
 */
template <typename F, typename... Args>
Task<typename std::result_of<F(Args...)>::type> run(F&& f, Args&&... args) {
    return {std::forward<F>(f), std::forward(args)...};
}
