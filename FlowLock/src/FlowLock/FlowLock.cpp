#include "FlowLock/FlowLock.h"
#include "FlowLock/FlowLockImpl.h"
#include "FlowLock/Utils/FlowTracer.h"
#include "FlowLock/Core/FlowBuilder.h"

namespace adapter {

bool FlowLock::await(std::chrono::milliseconds timeout) {
    return FlowLockImpl::instance().await(timeout);
}

void FlowLock::setThreadPoolSize(size_t size) {
    FlowLockImpl::instance().setThreadPoolSize(size);
}

void FlowLock::setPolicy(const std::string& tag, ConflictResolver::Policy policy) {
    FlowLockImpl::instance().setPolicy(tag, policy);
}

void FlowLock::setDefaultPolicy(ConflictResolver::Policy policy) {
    FlowLockImpl::instance().setDefaultPolicy(policy);
}

void FlowLock::shutdown() {
    FlowLockImpl::instance().shutdown();
}

FlowLock::Stats FlowLock::stats() {
    auto implStats = FlowLockImpl::instance().stats();
    return {
        implStats.queuedTaskCount,
        implStats.runningTaskCount,
        implStats.completedTaskCount,
        implStats.failedTaskCount,
        implStats.reEnqueuedCount
    };
}

std::string FlowLock::debugDump() {
    return FlowLockImpl::instance().debugDump();
}

FlowBuilder FlowLock::builder() {
    return FlowBuilder();
}

ScopedTask FlowLock::section(const std::string& name, uint32_t priority) {
    return ScopedTask(name, priority);
}

void FlowLock::enableTracing(bool enable) {
    FlowTracer::instance().setEnabled(enable);
}

bool FlowLock::exportTraceToJson(const std::string& filename) {
    return FlowTracer::instance().exportJsonToFile(filename);
}

void FlowLock::setAntiStarvationLimit(size_t limit) {
    FlowLockImpl::instance().setAntiStarvationLimit(limit);
}

} // namespace adapter