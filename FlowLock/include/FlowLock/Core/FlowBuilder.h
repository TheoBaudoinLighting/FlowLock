#pragma once

#include "FlowLock/Core/ConflictResolver.h"
#include "FlowLock/Context/FlowContext.h"
#include "FlowLock/FlowLockImpl.h"
#include <functional>
#include <future>
#include <string>
#include <vector>
#include <chrono>

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


template<typename F>
auto FlowBuilder::run(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>> {
    auto wrappedFunc = [func = std::forward<F>(func), timeout = this->timeout](FlowContext& ctx) mutable {
        if (timeout.count() > 0) {
            ctx.setTimeout(timeout);
        }
        return func(ctx);
    };

    if (hasCustomPolicy && !tags.empty()) {
        for (const auto& tag : tags) {
            FlowLockImpl::instance().setPolicy(tag, customPolicy);
        }
    }

    return FlowLockImpl::instance().request(std::move(wrappedFunc), priority, tags);
}

template<typename F>
auto FlowBuilder::operator<<(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>> {
    return run(std::forward<F>(func));
}

} // namespace adapter

