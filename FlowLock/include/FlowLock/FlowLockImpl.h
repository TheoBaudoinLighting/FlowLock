#pragma once

#include "FlowLock/Core/ConflictResolver.h"
#include <memory>
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace adapter {

// Forward declarations
class FlowExecution;
class FlowScheduler;
class FlowContext;
class FlowTask;
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
        -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>>;

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