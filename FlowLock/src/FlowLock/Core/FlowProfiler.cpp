#include "FlowLock/Core/FlowProfiler.h"
#include <sstream>
#include <iomanip>

namespace adapter {

    FlowProfiler& FlowProfiler::instance() {
        static FlowProfiler instance;
        return instance;
    }

    FlowProfiler::FlowProfiler() = default;

    void FlowProfiler::recordTaskExecution(const std::string& tag, uint32_t priority, std::chrono::nanoseconds duration) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.executionCount++;
        metric.totalExecutionTime += duration;

        if (duration < metric.minExecutionTime) {
            metric.minExecutionTime = duration;
        }

        if (duration > metric.maxExecutionTime) {
            metric.maxExecutionTime = duration;
        }

        if (metric.executionCount > 0) {
            metric.avgExecutionTime = metric.totalExecutionTime / metric.executionCount;
        }
    }

    void FlowProfiler::recordTaskQueued(const std::string& tag, uint32_t priority) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.queuedCount++;
    }

    void FlowProfiler::recordTaskCancelled(const std::string& tag, uint32_t priority) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.cancelledCount++;
    }

    void FlowProfiler::recordTaskTimedOut(const std::string& tag, uint32_t priority) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.timedOutCount++;
    }

    void FlowProfiler::recordTaskFailed(const std::string& tag, uint32_t priority) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.failedCount++;
    }

    void FlowProfiler::recordTaskReEnqueued(const std::string& tag, uint32_t priority) {
        if (!enabled) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto& metric = metrics[tag];
        metric.tag = tag;
        metric.priority = priority;
        metric.reEnqueuedCount++;
    }

    std::vector<TaskMetrics> FlowProfiler::getAllMetrics() const {
        std::lock_guard<std::mutex> lock(mutex);

        std::vector<TaskMetrics> result;
        result.reserve(metrics.size());

        for (const auto& [_, metric] : metrics) {
            result.push_back(metric);
        }

        return result;
    }

    std::optional<TaskMetrics> FlowProfiler::getMetricsForTag(const std::string& tag) const {
        std::lock_guard<std::mutex> lock(mutex);

        auto it = metrics.find(tag);
        if (it != metrics.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::string FlowProfiler::toJson() const {
        std::lock_guard<std::mutex> lock(mutex);

        std::stringstream ss;
        ss << "{";
        ss << "\"metrics\":[";

        bool first = true;
        for (const auto& [tag, metric] : metrics) {
            if (!first) {
                ss << ",";
            }
            first = false;

            ss << "{";
            ss << "\"tag\":\"" << tag << "\",";
            ss << "\"priority\":" << metric.priority << ",";
            ss << "\"executionCount\":" << metric.executionCount << ",";
            ss << "\"totalExecutionTimeMs\":" << std::chrono::duration_cast<std::chrono::milliseconds>(metric.totalExecutionTime).count() << ",";
            ss << "\"minExecutionTimeMs\":" << std::chrono::duration_cast<std::chrono::milliseconds>(metric.minExecutionTime).count() << ",";
            ss << "\"maxExecutionTimeMs\":" << std::chrono::duration_cast<std::chrono::milliseconds>(metric.maxExecutionTime).count() << ",";
            ss << "\"avgExecutionTimeMs\":" << std::chrono::duration_cast<std::chrono::milliseconds>(metric.avgExecutionTime).count() << ",";
            ss << "\"queuedCount\":" << metric.queuedCount << ",";
            ss << "\"cancelledCount\":" << metric.cancelledCount << ",";
            ss << "\"timedOutCount\":" << metric.timedOutCount << ",";
            ss << "\"failedCount\":" << metric.failedCount << ",";
            ss << "\"reEnqueuedCount\":" << metric.reEnqueuedCount;
            ss << "}";
        }

        ss << "]}";

        return ss.str();
    }

    std::string FlowProfiler::toPrometheusFormat() const {
        std::lock_guard<std::mutex> lock(mutex);

        std::stringstream ss;

        for (const auto& [tag, metric] : metrics) {
            ss << "flow_task_execution_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.executionCount << "\n";
            ss << "flow_task_total_execution_time_ms{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << std::chrono::duration_cast<std::chrono::milliseconds>(metric.totalExecutionTime).count() << "\n";
            ss << "flow_task_min_execution_time_ms{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << std::chrono::duration_cast<std::chrono::milliseconds>(metric.minExecutionTime).count() << "\n";
            ss << "flow_task_max_execution_time_ms{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << std::chrono::duration_cast<std::chrono::milliseconds>(metric.maxExecutionTime).count() << "\n";
            ss << "flow_task_avg_execution_time_ms{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << std::chrono::duration_cast<std::chrono::milliseconds>(metric.avgExecutionTime).count() << "\n";
            ss << "flow_task_queued_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.queuedCount << "\n";
            ss << "flow_task_cancelled_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.cancelledCount << "\n";
            ss << "flow_task_timed_out_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.timedOutCount << "\n";
            ss << "flow_task_failed_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.failedCount << "\n";
            ss << "flow_task_reenqueued_count{tag=\"" << tag << "\",priority=\"" << metric.priority << "\"} " << metric.reEnqueuedCount << "\n";
        }

        return ss.str();
    }

    void FlowProfiler::reset() {
        std::lock_guard<std::mutex> lock(mutex);
        metrics.clear();
    }

    bool FlowProfiler::isEnabled() const {
        return enabled;
    }

    void FlowProfiler::setEnabled(bool e) {
        enabled = e;
    }

} // namespace adapter