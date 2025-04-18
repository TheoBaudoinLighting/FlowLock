#pragma once

#include "FlowLock/Core/ConflictResolver.h"
#include <string>
#include <chrono>
#include <functional>
#include <future>
#include <vector>

namespace adapter {
    class FlowBuilder;
    class ScopedTask;
    class FlowContext;
    class FlowLockImpl;
}

namespace adapter {

class FlowLock {
public:
    static FlowLock& instance() {
        static FlowLock instance;
        return instance;
    }
    
    template<typename F>
    static auto run(F&& func, uint32_t priority = 0, const std::vector<std::string>& tags = {});
    
    template<typename F>
    static auto runExclusive(F&& func, const std::string& tag, uint32_t priority = 0);
    
    template<typename F>
    static auto runShared(F&& func, const std::string& tag, uint32_t priority = 0);
    
    template<typename F>
    static auto runPriority(F&& func, const std::string& tag, uint32_t priority);
    
    static bool await(std::chrono::milliseconds timeout = std::chrono::seconds(30));
    static void setThreadPoolSize(size_t size);
    static void setPolicy(const std::string& tag, ConflictResolver::Policy policy);
    static void setDefaultPolicy(ConflictResolver::Policy policy);
    static void shutdown();
    
    struct Stats {
        size_t queuedTaskCount;
        size_t runningTaskCount;
        size_t completedTaskCount;
        size_t failedTaskCount;
        size_t reEnqueuedCount;
    };
    
    static Stats stats();
    static std::string debugDump();
    static FlowBuilder builder();
    static ScopedTask section(const std::string& name, uint32_t priority = 0);
    
    static void enableTracing(bool enable);
    static bool exportTraceToJson(const std::string& filename);
    static void setAntiStarvationLimit(size_t limit);
};

} // namespace adapter