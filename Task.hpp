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
    void worker(Prom promise, F&& f, Args&&... args) {
        try {
            set_value(promise, std::forward<F>(f), std::forward(args)...);
        } catch (...) {
            try {
                promise.set_exception(std::current_exception());
            } catch (...) {
                // Ignore any other exceptions
            }
        }
        std::unique_lock<std::mutex> lock{mut};
        task_count--;
        cond.notify_all(); // notify_all, because multiple threads
                           // could call 'wait_all'
    }

public:
    /**
     * Obtain a handle to a statically initialized TaskManager.
     */
    static TaskManager& global() {
        static TaskManager manager;
        return manager;
    }

    ~TaskManager() { wait_all(); }

    /**
     * Creates an asynchronous task and returns a future which will contain
     * the result of the async computation. Note that this task will begin
     * immediately and that the destructor for this future will not block.
     * The thread will block until the completion of each task created by
     * a TaskManager, when that TaskManager is destructed (or during static
     * deinitialization, in the case of the 'global' instance)
     */
    template <typename F, typename... Args>
    std::future<std::result_of_t<F(Args...)>> spawn(F&& func, Args&&... args) {
        std::promise<std::result_of_t<F(Args...)>> promise;
        auto f = promise.get_future();

        task_count++;
        std::thread(&TaskManager::worker<decltype(promise), F, Args...>, this,
                    std::move(promise), std::forward<F>(func),
                    std::forward(args)...)
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
     * Block the calling thread until all tasks created by this TaskManager
     * have been completed.
     */
    void wait_all() {
        std::unique_lock<std::mutex> lock{mut};
        cond.wait(lock, [this] { return task_count == 0; });
    }
};

/**
 * A convenience alias for TaskManager::global().spawn()
 */
template <typename F, typename... Args>
inline std::future<std::result_of_t<F(Args...)>> spawn(F&& f, Args&&... args) {
    return TaskManager::global().spawn(std::forward<F>(f),
                                       std::forward(args)...);
}

/**
 * A convenience alias for TaskManager::spawn_with_result()
 */
template <typename T>
inline std::future<T> spawn_with_result(T&& result) {
    return TaskManager::spawn_with_result(std::forward<T>(result));
}
