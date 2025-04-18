#include "FlowLock/Utils/ThreadPool.h"

namespace adapter {

    ThreadPool::ThreadPool(size_t threads) {
        spawnWorkers(threads);
    }

    ThreadPool::~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutexQueue);
            stopping = true;
        }
        condTask.notify_all();
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }

    void ThreadPool::spawnWorkers(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutexQueue);
                        condTask.wait(lock, [this] { return stopping || !tasks.empty(); });
                        if (stopping && tasks.empty()) break;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    activeCount++;
                    task();
                    activeCount--;
                    {
                        std::unique_lock<std::mutex> lock(mutexQueue);
                        if (tasks.empty() && activeCount == 0) condFinish.notify_all();
                    }
                }
                });
        }
    }

    void ThreadPool::waitForTasks() {
        std::unique_lock<std::mutex> lock(mutexQueue);
        condFinish.wait(lock, [this] { return tasks.empty() && activeCount == 0; });
    }

    size_t ThreadPool::getQueueSize() const {
        std::unique_lock<std::mutex> lock(mutexQueue);
        return tasks.size();
    }

    size_t ThreadPool::getActiveThreadCount() const {
        return activeCount;
    }

    void ThreadPool::resize(size_t threads) {
        {
            std::unique_lock<std::mutex> lock(mutexQueue);
            stopping = true;
        }
        condTask.notify_all();
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
        workers.clear();
        stopping = false;
        spawnWorkers(threads);
    }

} // namespace adapter