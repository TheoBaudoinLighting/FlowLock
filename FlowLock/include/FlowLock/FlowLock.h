#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <future>
#include <vector>

#include "FlowLock/Core/ConflictResolver.h"
#include "FlowLock/Core/FlowBuilder.h"
#include "FlowLock/Core/FlowProfiler.h"
#include "FlowLock/Core/FlowSection.h"

#include "FlowLock/Context/FlowContext.h"
#include "FlowLock/FlowLockImpl.h"
#include "FlowLock/Utils/FlowTracer.h"

namespace adapter {

class FlowLock {
public:
    static FlowLock& instance() {
        static FlowLock instance;
        return instance;
    }
    
    template<typename F>
    static auto run(F&& func, uint32_t priority = 0, const std::vector<std::string>& tags = {}) {
        return FlowLockImpl::instance().request(std::forward<F>(func), priority, tags);
    }
    
    template<typename F>
    static auto runExclusive(F&& func, const std::string& tag, uint32_t priority = 0) {
        std::vector<std::string> tags = {tag};
        setPolicy(tag, ConflictResolver::Policy::EXCLUSIVE);
        return run(std::forward<F>(func), priority, tags);
    }
    
    template<typename F>
    static auto runShared(F&& func, const std::string& tag, uint32_t priority = 0) {
        std::vector<std::string> tags = {tag};
        setPolicy(tag, ConflictResolver::Policy::SHARED);
        return run(std::forward<F>(func), priority, tags);
    }
    
    template<typename F>
    static auto runPriority(F&& func, const std::string& tag, uint32_t priority) {
        std::vector<std::string> tags = {tag};
        setPolicy(tag, ConflictResolver::Policy::PRIORITY);
        return run(std::forward<F>(func), priority, tags);
    }
    
    static bool await(std::chrono::milliseconds timeout = std::chrono::seconds(30));
    static void setThreadPoolSize(size_t size);
    static void setPolicy(const std::string& tag, ConflictResolver::Policy policy);
    static void setDefaultPolicy(ConflictResolver::Policy policy);
    static void shutdown();
    static bool waitForDrain(std::chrono::milliseconds timeout = std::chrono::seconds(60));
    
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