#include "FlowTracer.h"
#include "FlowTask.h"
#include "FlowContext.h"

#include <iostream>

namespace adapter {

    std::atomic<uint32_t> FlowTracer::nextTaskId{ 0 };

    FlowTracer& FlowTracer::instance() {
        static FlowTracer instance;
        return instance;
    }

    FlowTracer::FlowTracer()
        : maxEvents(1000), enabled(true) {
    }

    void FlowTracer::recordTaskQueued(const std::shared_ptr<FlowTask>& task) {
        if (!enabled || !task) return;

        addEvent(
            TraceEvent::Type::TASK_QUEUED,
            "Task queued",
            nextTaskId++,
            std::nullopt,
            task->getTags()
        );
    }

    void FlowTracer::recordTaskStarted(const std::shared_ptr<FlowTask>& task, const FlowContext& context) {
        if (!enabled || !task) return;

        addEvent(
            TraceEvent::Type::TASK_STARTED,
            "Task started",
            nextTaskId++,
            context.getThreadId(),
            task->getTags()
        );
    }

    void FlowTracer::recordTaskCompleted(const std::shared_ptr<FlowTask>& task, const FlowContext& context) {
        if (!enabled || !task) return;

        std::string desc = "Task completed";
        if (context.isProfilingEnabled()) {
            auto profile = context.getLastProfileData();
            if (profile) {
                auto duration = profile->duration();
                desc += " (duration: " +
                    std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()) +
                    " ms)";
            }
        }

        addEvent(
            TraceEvent::Type::TASK_COMPLETED,
            desc,
            nextTaskId++,
            context.getThreadId(),
            task->getTags()
        );
    }

    void FlowTracer::recordTaskFailed(const std::shared_ptr<FlowTask>& task, const FlowContext& context, const std::string& error) {
        if (!enabled || !task) return;

        try {
            std::cerr << "Recording task failure: " << error << std::endl;

            addEvent(
                TraceEvent::Type::TASK_FAILED,
                "Task failed: " + error,
                nextTaskId++,
                context.getThreadId(),
                task->getTags()
            );
        }
        catch (const std::exception& e) {
            std::cerr << "Erreur dans recordTaskFailed: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Erreur inconnue dans recordTaskFailed" << std::endl;
        }
    }

    void FlowTracer::recordSchedulerEmpty() {
        if (!enabled) return;

        addEvent(
            TraceEvent::Type::SCHEDULER_EMPTY,
            "Scheduler queue empty"
        );
    }

    void FlowTracer::recordConflictDetected(const std::shared_ptr<FlowTask>& task, const std::string& reason) {
        if (!enabled || !task) return;

        addEvent(
            TraceEvent::Type::CONFLICT_DETECTED,
            "Conflict detected: " + reason,
            nextTaskId++,
            std::nullopt,
            task->getTags()
        );
    }

    void FlowTracer::addEvent(TraceEvent::Type type, const std::string& description,
        std::optional<uint32_t> taskId,
        std::optional<uint32_t> threadId,
        const std::vector<std::string>& tags) {
        if (!enabled) return;

        try {
            std::lock_guard<std::mutex> lock(mutex);

            TraceEvent event;
            event.type = type;
            event.timestamp = std::chrono::steady_clock::now();
            event.description = description;
            event.taskId = taskId;
            event.threadId = threadId;
            event.tags = tags;

            events.push_back(event);

            if (events.size() > maxEvents) {
                events.erase(events.begin());
            }

            if (type == TraceEvent::Type::TASK_FAILED) {
                std::cerr << "FlowTracer: Task failed event recorded: " << description << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in FlowTracer::addEvent: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unknown error in FlowTracer::addEvent" << std::endl;
        }
    }

    const std::vector<TraceEvent>& FlowTracer::getEvents() const {
        std::lock_guard<std::mutex> lock(mutex);
        return events;
    }

    void FlowTracer::clear() {
        std::lock_guard<std::mutex> lock(mutex);
        events.clear();
    }

    void FlowTracer::setMaxEvents(size_t max) {
        std::lock_guard<std::mutex> lock(mutex);
        maxEvents = max;

        if (events.size() > maxEvents) {
            events.erase(events.begin(), events.begin() + (events.size() - maxEvents));
        }
    }

    bool FlowTracer::isEnabled() const {
        return enabled;
    }

    void FlowTracer::setEnabled(bool enable) {
        enabled = enable;
    }

} // namespace adapter