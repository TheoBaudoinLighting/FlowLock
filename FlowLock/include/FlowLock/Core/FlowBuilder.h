#pragma once

#include "FlowLock/Core/ConflictResolver.h"
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <chrono>

namespace adapter {
    class FlowContext;
    class FlowLockImpl;
}

namespace adapter {

class FlowBuilder {
public:
    FlowBuilder();

    FlowBuilder& withPriority(uint32_t priority);
    FlowBuilder& withTag(const std::string& tag);
    FlowBuilder& withTags(const std::vector<std::string>& tags);
    FlowBuilder& withTimeout(std::chrono::milliseconds timeout);
    FlowBuilder& exclusive();
    FlowBuilder& shared();
    FlowBuilder& prioritized();

    template<typename F>
    auto run(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>>;

    template<typename F>
    auto operator<<(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>>;

private:
    uint32_t priority{ 0 };
    std::vector<std::string> tags;
    std::chrono::milliseconds timeout{ 0 };
    bool hasCustomPolicy{ false };
    ConflictResolver::Policy customPolicy;
};

class ScopedTask {
public:
    ScopedTask(const std::string& name, uint32_t priority = 0);
    ~ScopedTask();

    template<typename F>
    auto operator<<(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>>;

private:
    std::string taskName;
    uint32_t priority;
    std::vector<std::string> tags;
};

} // namespace adapter