/*
 * C++11 introduced concurrency features like std::async, future and promise,
 * and while these tools are often sufficient and useful, they can sometimes
 * behave in unexpected ways.
 *
 * Specifically, a std::future returned from std::async with the 'async'
 * launch policy will block (i.e., join the underlying thread) when its
 * destructor is called. This is often undesirable. The 'run' function
 * in this file has the following benefits over std::async.
 *
 * 1. It will always run the task asynchronously, no launch policy is necessary
 * 2. The underlying thread is detached, so no blocking will occur unless the
 * user calls 'get' explicitly.
 * 3. The program will not terminate until all running tasks are finished.
 *
 * Note: that the blocking required for '3' occurs during static
 * de-initialization. It is possible, therefore, that a task making use of
 * static variables may invoke undefined behavior if the variable is
 * destructed while the task is still running.
 *
 * To avoid this behavior, the user can invoke "TaskManager::wait_all" before
 * the end of 'main'.
 */

#include <future>

class TaskManager {
    std::mutex mut;
    std::condition_variable cond;
    std::atomic<int> task_count{0};

    // As the TaskManager is a singleton, this will occur during static
    // deinitialization, and block until all tasks have finished.
    ~TaskManager() { wait_all(); }

    template <typename R, typename F, typename... Args>
    static void set_value(std::promise<R>& promise, F&& f, Args&&... args) {
        promise.set_value(f(args...));
    }

    // If the function returns void, do not try to set the promise
    template <typename F, typename... Args>
    static void set_value(std::promise<void>&, F&& f, Args&&... args) {
        f(args...);
    }

    template <typename Prom, typename F, typename... Args>
    static void worker(Prom promise, F&& f, Args&&... args) {
        try {
            set_value(promise, std::forward<F>(f), std::forward(args)...);
        } catch (...) {
            try {
                promise.set_exception(std::current_exception());
            } catch (...) {
                // Ignore any other exceptions
            }
        }
        std::unique_lock<std::mutex> lock{instance().mut};
        instance().task_count--;
        instance().cond.notify_all(); // notify_all, because multiple threads
                                      // could call 'wait_all'
    }

    static TaskManager& instance() {
        static TaskManager manager;
        return manager;
    }

public:
    /**
     * Creates an asynchronous task and returns a future which will contain
     * the result of the async computation. Note that this task will begin
     * immediately and that the program will not exit until all such tasks
     * are completed.
     */
    template <typename F, typename... Args>
    static std::future<std::result_of_t<F(Args...)>> spawn(F&& func,
                                                           Args&&... args) {
        std::promise<std::result_of_t<F(Args...)>> promise;
        auto f = promise.get_future();

        instance().task_count++;
        std::thread(worker<decltype(promise), F, Args...>, std::move(promise),
                    std::forward<F>(func), std::forward(args)...)
            .detach();
        return f;
    }

    /**
     * Create a 'ready' future with the given value. Calling get/wait on this
     * future or allowing this future to be destroyed will not block.
     */
    template <typename T>
    static std::future<T> spawn_with_result(T&& result) {
        std::promise<T> promise;
        promise.set_value(std::forward<T>(result));
        return promise.get_future();
    }

    /**
     * Block the calling thread until all running tasks have completed.
     */
    static void wait_all() {
        std::unique_lock<std::mutex> lock{instance().mut};
        instance().cond.wait(lock, [] { return instance().task_count == 0; });
    }
};

/**
 * Analogous to 'std::async'. However, futures returned from 'run' will
 * always be executing asynchronously (no launch policy is required),
 * and the main thread will not exit until all such tasks are completed.
 */
template <typename F, typename... Args>
inline std::future<std::result_of_t<F(Args...)>> run(F&& f, Args&&... args) {
    return TaskManager::spawn(std::forward<F>(f), std::forward(args)...);
}
