#pragma once

#include <memory>
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "FlowScheduler.h"
#include "FlowTask.h"
#include "ThreadPool.h"

namespace Volvic::Ticking {

    class FlowExecution;
    class ConflictResolver;
    class FlowContext;

    class FlowLock {
    public:
        static FlowLock& instance();

        FlowLock(const FlowLock&) = delete;
        FlowLock(FlowLock&&) = delete;
        FlowLock& operator=(const FlowLock&) = delete;
        FlowLock& operator=(FlowLock&&) = delete;

        template<typename F>
        auto request(F&& func, uint32_t priority = 0, const std::vector<std::string>& tags = {})
            -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>>;

        void await();
        void run();

        void setThreadPoolSize(size_t threads);

        using TaskCompletionCallback = std::function<void(const std::shared_ptr<FlowTask>&)>;
        void setTaskCompletionCallback(TaskCompletionCallback callback);

    private:
        FlowLock();
        ~FlowLock();

        std::unique_ptr<FlowScheduler> scheduler;
        std::unique_ptr<FlowExecution> execution;
        std::unique_ptr<ConflictResolver> conflictResolver;
        std::unique_ptr<ThreadPool> threadPool;

        std::mutex processMutex;
        std::condition_variable scheduleCondVar;
        TaskCompletionCallback userCompletionCallback;
        bool allTasksCompleted{ true };

        void processNextTask();
        void onTaskCompleted(const std::shared_ptr<FlowTask>& task);
    };

    template<typename F>
    auto FlowLock::request(F&& func, uint32_t priority, const std::vector<std::string>& tags) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>> {
        using ReturnType = std::invoke_result_t<std::decay_t<F>, FlowContext&>;

        auto packagedTask = std::make_shared<std::packaged_task<ReturnType(FlowContext&)>>(
            std::forward<F>(func)
        );

        std::future<ReturnType> result = packagedTask->get_future();

        auto task = std::make_shared<FlowTask>(
            [packagedTask](FlowContext& ctx) {
                (*packagedTask)(ctx);
            },
            priority,
            std::chrono::steady_clock::now()
        );

        for (const auto& tag : tags) {
            task->addTag(tag);
        }

        scheduler->enqueueTask(task);

        return result;
    }

} // namespace Volvic::Ticking