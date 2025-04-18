#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include <optional>

namespace adapter {

    struct TaskMetrics {
        std::string tag;
        uint32_t priority;
        size_t executionCount{ 0 };
        std::chrono::nanoseconds totalExecutionTime{ 0 };
        std::chrono::nanoseconds minExecutionTime{ std::chrono::hours(24) };
        std::chrono::nanoseconds maxExecutionTime{ 0 };
        std::chrono::nanoseconds avgExecutionTime{ 0 };

        size_t queuedCount{ 0 };
        size_t cancelledCount{ 0 };
        size_t timedOutCount{ 0 };
        size_t failedCount{ 0 };
        size_t reEnqueuedCount{ 0 };
    };

    class FlowProfiler {
    public:
        static FlowProfiler& instance();

        FlowProfiler(const FlowProfiler&) = delete;
        FlowProfiler(FlowProfiler&&) = delete;
        FlowProfiler& operator=(const FlowProfiler&) = delete;
        FlowProfiler& operator=(FlowProfiler&&) = delete;

        void recordTaskExecution(const std::string& tag, uint32_t priority, std::chrono::nanoseconds duration);
        void recordTaskQueued(const std::string& tag, uint32_t priority);
        void recordTaskCancelled(const std::string& tag, uint32_t priority);
        void recordTaskTimedOut(const std::string& tag, uint32_t priority);
        void recordTaskFailed(const std::string& tag, uint32_t priority);
        void recordTaskReEnqueued(const std::string& tag, uint32_t priority);

        std::vector<TaskMetrics> getAllMetrics() const;
        std::optional<TaskMetrics> getMetricsForTag(const std::string& tag) const;

        std::string toJson() const;
        std::string toPrometheusFormat() const;

        void reset();

        bool isEnabled() const;
        void setEnabled(bool enabled);

    private:
        FlowProfiler();

        mutable std::mutex mutex;
        std::unordered_map<std::string, TaskMetrics> metrics;
        bool enabled{ true };
    };

} // namespace adapter