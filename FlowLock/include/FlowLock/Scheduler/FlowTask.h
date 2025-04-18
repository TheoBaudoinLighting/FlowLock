#pragma once

#include <functional>
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>

namespace adapter {
    class FlowContext;
}

namespace adapter {

class FlowTask {
public:
    using TaskFunction = std::function<void(FlowContext&)>;

    FlowTask(TaskFunction function, uint32_t priority = 0,
             std::chrono::steady_clock::time_point timestamp = std::chrono::steady_clock::now());

    void addTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::vector<std::string>& getTags() const;

    uint32_t getPriority() const;
    std::chrono::steady_clock::time_point getTimestamp() const;

    void execute(FlowContext& context);
    
    void cancel();
    bool isCancelled() const;
    
    void setTimeout(std::chrono::milliseconds timeout);
    bool isTimedOut() const;
    
    void incrementReenqueueCount();
    size_t getReenqueueCount() const;

private:
    TaskFunction function;
    uint32_t priority;
    std::chrono::steady_clock::time_point timestamp;
    std::vector<std::string> tags;
    std::atomic<bool> cancelled{false};
    std::optional<std::chrono::steady_clock::time_point> deadlineTime;
    std::atomic<size_t> reenqueueCount{0};
};

} // namespace adapter