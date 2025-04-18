// FlowTask.cpp
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Context/FlowContext.h"
#include <algorithm>

namespace adapter {

FlowTask::FlowTask(TaskFunction function, uint32_t priority,
    std::chrono::steady_clock::time_point timestamp)
    : function(function), priority(priority), timestamp(timestamp) {
}

void FlowTask::addTag(const std::string& tag) {
    if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
        tags.push_back(tag);
    }
}

bool FlowTask::hasTag(const std::string& tag) const {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

const std::vector<std::string>& FlowTask::getTags() const {
    return tags;
}

uint32_t FlowTask::getPriority() const {
    return priority;
}

std::chrono::steady_clock::time_point FlowTask::getTimestamp() const {
    return timestamp;
}

void FlowTask::execute(FlowContext& context) {
    if (cancelled || isTimedOut()) {
        return;
    }
    
    if (function) {
        function(context);
    }
}

void FlowTask::cancel() {
    cancelled = true;
}

bool FlowTask::isCancelled() const {
    return cancelled;
}

void FlowTask::setTimeout(std::chrono::milliseconds timeout) {
    if (timeout.count() > 0) {
        deadlineTime = std::chrono::steady_clock::now() + timeout;
    } else {
        deadlineTime.reset();
    }
}

bool FlowTask::isTimedOut() const {
    if (!deadlineTime) return false;
    return std::chrono::steady_clock::now() > *deadlineTime;
}

void FlowTask::incrementReenqueueCount() {
    reenqueueCount++;
}

size_t FlowTask::getReenqueueCount() const {
    return reenqueueCount;
}

} // namespace adapter