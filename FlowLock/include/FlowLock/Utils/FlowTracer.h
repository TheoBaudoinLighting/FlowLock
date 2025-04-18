#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <optional>
#include <atomic>
#include <fstream>

namespace adapter {

class FlowTask;
class FlowContext;

struct TraceEvent {
    enum class Type {
        TASK_QUEUED,
        TASK_STARTED,
        TASK_COMPLETED,
        TASK_FAILED,
        SCHEDULER_EMPTY,
        CONFLICT_DETECTED,
        TASK_CANCELLED,
        TASK_TIMED_OUT,
        ANTI_STARVATION_APPLIED
    };

    Type type;
    std::chrono::steady_clock::time_point timestamp;
    std::string description;
    std::optional<uint32_t> taskId;
    std::optional<uint32_t> threadId;
    std::vector<std::string> tags;
    uint32_t priority{0};
};

class FlowTracer {
public:
    static FlowTracer& instance();

    FlowTracer(const FlowTracer&) = delete;
    FlowTracer(FlowTracer&&) = delete;
    FlowTracer& operator=(const FlowTracer&) = delete;
    FlowTracer& operator=(FlowTracer&&) = delete;

    void recordTaskQueued(const std::shared_ptr<FlowTask>& task);
    void recordTaskStarted(const std::shared_ptr<FlowTask>& task, const FlowContext& context);
    void recordTaskCompleted(const std::shared_ptr<FlowTask>& task, const FlowContext& context);
    void recordTaskFailed(const std::shared_ptr<FlowTask>& task, const FlowContext& context, const std::string& error);
    void recordSchedulerEmpty();
    void recordConflictDetected(const std::shared_ptr<FlowTask>& task, const std::string& reason);
    void recordTaskCancelled(const std::shared_ptr<FlowTask>& task);
    void recordTaskTimedOut(const std::shared_ptr<FlowTask>& task);
    void recordAntiStarvationApplied(const std::shared_ptr<FlowTask>& task, size_t reenqueueCount);

    void addEvent(TraceEvent::Type type, const std::string& description,
        std::optional<uint32_t> taskId = std::nullopt,
        std::optional<uint32_t> threadId = std::nullopt,
        const std::vector<std::string>& tags = {},
        uint32_t priority = 0);

    const std::vector<TraceEvent>& getEvents() const;
    void clear();

    void setMaxEvents(size_t max);

    bool isEnabled() const;
    void setEnabled(bool enable);
    
    std::string toJson() const;
    bool exportJsonToFile(const std::string& filename) const;

private:
    FlowTracer();

    mutable std::mutex mutex;
    std::vector<TraceEvent> events;
    size_t maxEvents;
    bool enabled;

    static std::atomic<uint32_t> nextTaskId;
};

} // namespace adapter