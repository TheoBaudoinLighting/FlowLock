#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <stdexcept>

namespace adapter {

    class ThreadPool {
    public:
        ThreadPool(size_t threads = std::thread::hardware_concurrency());
        ~ThreadPool();

        template<typename F, typename... Args>
        auto enqueue(F&& f, Args&&... args)
            -> std::future<typename std::invoke_result_t<F, Args...>>;

        void waitForTasks();
        size_t getQueueSize() const;
        size_t getActiveThreadCount() const;
        void resize(size_t threads);

    private:
        void spawnWorkers(size_t count);

        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        mutable std::mutex mutexQueue;
        std::condition_variable condTask;
        std::condition_variable condFinish;
        std::atomic<bool> stopping{ false };
        std::atomic<size_t> activeCount{ 0 };
    };

    template<typename F, typename... Args>
    auto ThreadPool::enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>
    {
        using ReturnType = typename std::invoke_result_t<F, Args...>;
        auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> res = taskPtr->get_future();
        {
            std::unique_lock<std::mutex> lock(mutexQueue);
            if (stopping) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([taskPtr]() { (*taskPtr)(); });
        }
        condTask.notify_one();
        return res;
    }

} // namespace adapter