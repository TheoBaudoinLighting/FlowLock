#include "FlowLock/Core/FlowBuilder.h"
#include "FlowLock/FlowLockImpl.h"
#include "FlowLock/Context/FlowContext.h"

namespace adapter {

FlowBuilder::FlowBuilder() = default;

FlowBuilder& FlowBuilder::withPriority(uint32_t p) {
    priority = p;
    return *this;
}

FlowBuilder& FlowBuilder::withTag(const std::string& tag) {
    tags.push_back(tag);
    return *this;
}

FlowBuilder& FlowBuilder::withTags(const std::vector<std::string>& t) {
    tags.insert(tags.end(), t.begin(), t.end());
    return *this;
}

FlowBuilder& FlowBuilder::withTimeout(std::chrono::milliseconds t) {
    timeout = t;
    return *this;
}

FlowBuilder& FlowBuilder::exclusive() {
    hasCustomPolicy = true;
    customPolicy = ConflictResolver::Policy::EXCLUSIVE;
    return *this;
}

FlowBuilder& FlowBuilder::shared() {
    hasCustomPolicy = true;
    customPolicy = ConflictResolver::Policy::SHARED;
    return *this;
}

FlowBuilder& FlowBuilder::prioritized() {
    hasCustomPolicy = true;
    customPolicy = ConflictResolver::Policy::PRIORITY;
    return *this;
}

ScopedTask::ScopedTask(const std::string& name, uint32_t p)
    : taskName(name), priority(p) {
    tags.push_back("section:" + name);
}

ScopedTask::~ScopedTask() = default;

template<typename F>
auto ScopedTask::operator<<(F&& func) -> std::future<std::invoke_result_t<std::decay_t<F>, FlowContext&>> {
    tags.push_back("section:" + taskName);
    auto wrappedFunc = [func = std::forward<F>(func), name = taskName](FlowContext& ctx) mutable {
        ctx.startProfiling(name);
        auto result = func(ctx);
        ctx.endProfiling();
        return result;
    };

    return FlowLockImpl::instance().request(std::move(wrappedFunc), priority, tags);
}

} // namespace adapter