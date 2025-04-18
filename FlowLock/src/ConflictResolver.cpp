#include "ConflictResolver.h"
#include "FlowTask.h"
#include "FlowTracer.h"
#include <algorithm>
#include <sstream>
#include <iostream> 

namespace adapter {

    ConflictResolver::ConflictResolver()
        : defaultPolicy(Policy::SHARED) {
    }

    void ConflictResolver::setPolicy(const std::string& tag, Policy policy) {
        policies[tag] = policy;
    }

    ConflictResolver::Policy ConflictResolver::getPolicy(const std::string& tag) const {
        auto it = policies.find(tag);
        return (it != policies.end()) ? it->second : defaultPolicy;
    }

    bool ConflictResolver::canExecute(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const {
        if (!task || runningTasks.empty()) {
            return true;
        }

        const auto& tags = task->getTags();

        if (tags.empty()) {
            return true;
        }

        for (const auto& tag : tags) {
            auto policy = getPolicy(tag);

            if (policy == Policy::EXCLUSIVE) {
                for (const auto& runningTask : runningTasks) {
                    const auto& runningTaskTags = runningTask->getTags();
                    if (std::find(runningTaskTags.begin(), runningTaskTags.end(), tag) != runningTaskTags.end()) {
                        std::stringstream reason;
                        reason << "Exclusive tag conflict on '" << tag << "'";
                        try {
                            FlowTracer::instance().recordConflictDetected(task, reason.str());
                        }
                        catch (...) {}
                        return false;
                    }
                }
            }
            else if (policy == Policy::PRIORITY) {
                for (const auto& runningTask : runningTasks) {
                    const auto& runningTaskTags = runningTask->getTags();
                    if (std::find(runningTaskTags.begin(), runningTaskTags.end(), tag) != runningTaskTags.end() &&
                        task->getPriority() <= runningTask->getPriority()) {
                        std::stringstream reason;
                        reason << "Priority conflict on tag '" << tag << "': "
                            << "Current task (priority " << task->getPriority()
                            << ") <= Running task (priority " << runningTask->getPriority() << ")";
                        try {
                            FlowTracer::instance().recordConflictDetected(task, reason.str());
                        }
                        catch (...) {}
                        return false;
                    }
                }
            }
            // For SHARED policy, we don't need to check anything
        }

        return true;
    }

    bool ConflictResolver::checkExclusiveConflict(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const {
        const auto& taskTags = task->getTags();

        for (const auto& runningTask : runningTasks) {
            const auto& runningTaskTags = runningTask->getTags();

            for (const auto& tag : taskTags) {
                if (std::find(runningTaskTags.begin(), runningTaskTags.end(), tag) != runningTaskTags.end()) {
                    std::stringstream reason;
                    reason << "Exclusive tag conflict on '" << tag << "'";
                    FlowTracer::instance().recordConflictDetected(task, reason.str());

                    return false;
                }
            }
        }

        return true;
    }

    bool ConflictResolver::checkPriorityConflict(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const {
        const auto& taskTags = task->getTags();

        for (const auto& runningTask : runningTasks) {
            const auto& runningTaskTags = runningTask->getTags();

            bool hasTagOverlap = false;
            std::string overlappingTag;
            for (const auto& tag : taskTags) {
                if (std::find(runningTaskTags.begin(), runningTaskTags.end(), tag) != runningTaskTags.end()) {
                    hasTagOverlap = true;
                    overlappingTag = tag;
                    break;
                }
            }

            if (hasTagOverlap && task->getPriority() <= runningTask->getPriority()) {
                std::stringstream reason;
                reason << "Priority conflict on tag '" << overlappingTag << "': "
                    << "Current task (priority " << task->getPriority()
                    << ") <= Running task (priority " << runningTask->getPriority() << ")";
                FlowTracer::instance().recordConflictDetected(task, reason.str());

                return false;
            }
        }

        return true;
    }

} // namespace adapter