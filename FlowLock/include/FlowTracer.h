#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <optional>

namespace Volvic::Ticking {

    class FlowTask;
    class FlowContext;

    struct TraceEvent {
        enum class Type {
            TASK_QUEUED,
            TASK_STARTED,
            TASK_COMPLETED,
            TASK_FAILED,
            SCHEDULER_EMPTY,
            CONFLICT_DETECTED
        };

        Type type;
        std::chrono::steady_clock::time_point timestamp;
        std::string description;
        std::optional<uint32_t> taskId;
        std::optional<uint32_t> threadId;
        std::vector<std::string> tags;
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

        void addEvent(TraceEvent::Type type, const std::string& description,
            std::optional<uint32_t> taskId = std::nullopt,
            std::optional<uint32_t> threadId = std::nullopt,
            const std::vector<std::string>& tags = {});

        const std::vector<TraceEvent>& getEvents() const;
        void clear();

        void setMaxEvents(size_t max);

        bool isEnabled() const;
        void setEnabled(bool enable);

    private:
        FlowTracer();

        mutable std::mutex mutex;
        std::vector<TraceEvent> events;
        size_t maxEvents;
        bool enabled;

        static std::atomic<uint32_t> nextTaskId;
    };

} // namespace Volvic::Ticking