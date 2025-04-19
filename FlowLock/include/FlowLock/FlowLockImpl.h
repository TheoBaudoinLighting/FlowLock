#pragma once

#include "FlowLock/Core/ConflictResolver.h"
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Scheduler/FlowScheduler.h"
#include "FlowLock/Execution/FlowExecution.h"
#include "FlowLock/Context/FlowContext.h"
#include "FlowLock/Utils/ThreadPool.h"
#include <memory>
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>

namespace adapter {

class FlowExecution;
class FlowScheduler;
class FlowContext;
class ThreadPool;

class FlowLockImpl {
public:
    static FlowLockImpl& instance();

    FlowLockImpl(const FlowLockImpl&) = delete;
    FlowLockImpl(FlowLockImpl&&) = delete;
    FlowLockImpl& operator=(const FlowLockImpl&) = delete;
    FlowLockImpl& operator=(FlowLockImpl&&) = delete;

    template<typename F>
    auto request(F&& func, uint32_t priority = 0, const std::vector<std::string>& tags = {})
        -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>> {
        using ReturnType = std::invoke_result_t<std::decay_t<F>, FlowContext&>;
        auto taskPromise = std::make_shared<std::promise<ReturnType>>();
        auto future = taskPromise->get_future();
        
        auto task = std::make_shared<FlowTask>(
            [func = std::forward<F>(func), promise = taskPromise](FlowContext& context) mutable {
                try {
                    if constexpr (std::is_void_v<ReturnType>) {
                        func(context);
                        promise->set_value();
                    } else {
                        promise->set_value(func(context));
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            },
            priority
        );
        
        for (const auto& tag : tags) {
            task->addTag(tag);
        }
        
        scheduler->enqueueTask(task);
        scheduleCondVar.notify_one();
        
        return future;
    }

    bool await(std::chrono::milliseconds timeout = std::chrono::seconds(5));
    void run();
    void shutdown();

    void setThreadPoolSize(size_t threads);

    using TaskCompletionCallback = std::function<void(const std::shared_ptr<FlowTask>&)>;
    void setTaskCompletionCallback(TaskCompletionCallback callback);
    
    void setPolicy(const std::string& tag, ConflictResolver::Policy policy);
    void setDefaultPolicy(ConflictResolver::Policy policy);
    
    struct Stats {
        size_t queuedTaskCount;
        size_t runningTaskCount;
        size_t completedTaskCount;
        size_t failedTaskCount;
        size_t reEnqueuedCount;
    };
    
    Stats stats() const;
    std::string debugDump() const;
    
    void setAntiStarvationLimit(size_t limit);
    size_t getAntiStarvationLimit() const;

    std::unique_ptr<FlowScheduler>& getScheduler() { return scheduler; }
    std::unique_ptr<FlowExecution>& getExecution() { return execution; }
    std::unique_ptr<ConflictResolver>& getConflictResolver() { return conflictResolver; }

private:
    FlowLockImpl();
    ~FlowLockImpl();

    std::unique_ptr<FlowScheduler> scheduler;
    std::unique_ptr<FlowExecution> execution;
    std::unique_ptr<ConflictResolver> conflictResolver;
    std::unique_ptr<ThreadPool> threadPool;

    std::atomic<bool> stopping{false};
    std::mutex processMutex;
    std::condition_variable scheduleCondVar;
    TaskCompletionCallback userCompletionCallback;
    bool allTasksCompleted{true};
    
    std::atomic<size_t> completedTaskCount{0};
    std::atomic<size_t> failedTaskCount{0};
    std::atomic<size_t> reEnqueuedTaskCount{0};
    
    size_t antiStarvationLimit{10};

    void processNextTask();
    void onTaskCompleted(const std::shared_ptr<FlowTask>& task);
};

} // namespace adapter