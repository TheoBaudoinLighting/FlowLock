#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace adapter {

class FlowTask;

class ConflictResolver {
public:
    enum class Policy {
        EXCLUSIVE,  // Only one task with a specific tag can run at a time
        SHARED,     // Multiple tasks with the same tag can run concurrently
        PRIORITY    // Higher priority tasks preempt lower priority ones
    };

    ConflictResolver();

    void setPolicy(const std::string& tag, Policy policy);
    Policy getPolicy(const std::string& tag) const;

    bool canExecute(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const;

private:
    std::unordered_map<std::string, Policy> policies;
    Policy defaultPolicy;

    bool checkExclusiveConflict(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const;

    bool checkPriorityConflict(const std::shared_ptr<FlowTask>& task,
        const std::vector<std::shared_ptr<FlowTask>>& runningTasks) const;
};

} // namespace adapter