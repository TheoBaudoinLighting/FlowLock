#include "FlowTask.h"
#include "FlowContext.h"
#include <algorithm>

namespace Volvic::Ticking {

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
        if (function) {
            function(context);
        }
    }

} // namespace Volvic::Ticking