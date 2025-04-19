#include "FlowLock/Utils/FlowTracer.h"
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Context/FlowContext.h"
#include <iostream>
#include <fstream>
#include <sstream>

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
        task->getTags(),
        task->getPriority()
    );
}

void FlowTracer::recordTaskStarted(const std::shared_ptr<FlowTask>& task, const FlowContext& context) {
    if (!enabled || !task) return;

    addEvent(
        TraceEvent::Type::TASK_STARTED,
        "Task started",
        nextTaskId++,
        context.getThreadId(),
        task->getTags(),
        task->getPriority()
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
                " μs)";
        }
    }

    addEvent(
        TraceEvent::Type::TASK_COMPLETED,
        desc,
        nextTaskId++,
        context.getThreadId(),
        task->getTags(),
        task->getPriority()
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
            task->getTags(),
            task->getPriority()
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
        task->getTags(),
        task->getPriority()
    );
}

void FlowTracer::recordTaskCancelled(const std::shared_ptr<FlowTask>& task) {
    if (!enabled || !task) return;

    addEvent(
        TraceEvent::Type::TASK_CANCELLED,
        "Task cancelled",
        nextTaskId++,
        std::nullopt,
        task->getTags(),
        task->getPriority()
    );
}

void FlowTracer::recordTaskTimedOut(const std::shared_ptr<FlowTask>& task) {
    if (!enabled || !task) return;

    addEvent(
        TraceEvent::Type::TASK_TIMED_OUT,
        "Task timed out",
        nextTaskId++,
        std::nullopt,
        task->getTags(),
        task->getPriority()
    );
}

void FlowTracer::recordAntiStarvationApplied(const std::shared_ptr<FlowTask>& task, size_t reenqueueCount) {
    if (!enabled || !task) return;

    std::string desc = "Anti-starvation applied after " + std::to_string(reenqueueCount) + " re-enqueues";
    
    addEvent(
        TraceEvent::Type::ANTI_STARVATION_APPLIED,
        desc,
        nextTaskId++,
        std::nullopt,
        task->getTags(),
        task->getPriority()
    );
}

void FlowTracer::addEvent(TraceEvent::Type type, const std::string& description,
    std::optional<uint32_t> taskId,
    std::optional<uint32_t> threadId,
    const std::vector<std::string>& tags,
    uint32_t priority) {
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
        event.priority = priority;

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

std::string FlowTracer::toJson() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::stringstream ss;
    ss << "{\"events\":[";
    
    bool first = true;
    for (const auto& event : events) {
        if (!first) {
            ss << ",";
        }
        first = false;
        
        ss << "{";
        ss << "\"type\":\"";
        switch (event.type) {
            case TraceEvent::Type::TASK_QUEUED: ss << "TASK_QUEUED"; break;
            case TraceEvent::Type::TASK_STARTED: ss << "TASK_STARTED"; break;
            case TraceEvent::Type::TASK_COMPLETED: ss << "TASK_COMPLETED"; break;
            case TraceEvent::Type::TASK_FAILED: ss << "TASK_FAILED"; break;
            case TraceEvent::Type::SCHEDULER_EMPTY: ss << "SCHEDULER_EMPTY"; break;
            case TraceEvent::Type::CONFLICT_DETECTED: ss << "CONFLICT_DETECTED"; break;
            case TraceEvent::Type::TASK_CANCELLED: ss << "TASK_CANCELLED"; break;
            case TraceEvent::Type::TASK_TIMED_OUT: ss << "TASK_TIMED_OUT"; break;
            case TraceEvent::Type::ANTI_STARVATION_APPLIED: ss << "ANTI_STARVATION_APPLIED"; break;
            default: ss << "UNKNOWN"; break;
        }
        ss << "\",";
        
        ss << "\"timestamp\":\"" << std::chrono::duration_cast<std::chrono::milliseconds>(
            event.timestamp.time_since_epoch()).count() << "\",";
        ss << "\"description\":\"" << event.description << "\",";
        
        if (event.taskId) {
            ss << "\"taskId\":" << *event.taskId << ",";
        } else {
            ss << "\"taskId\":null,";
        }
        
        if (event.threadId) {
            ss << "\"threadId\":" << *event.threadId << ",";
        } else {
            ss << "\"threadId\":null,";
        }
        
        ss << "\"priority\":" << event.priority << ",";
        
        ss << "\"tags\":[";
        bool firstTag = true;
        for (const auto& tag : event.tags) {
            if (!firstTag) {
                ss << ",";
            }
            firstTag = false;
            ss << "\"" << tag << "\"";
        }
        ss << "]";
        
        ss << "}";
    }
    
    ss << "]}";
    
    return ss.str();
}

bool FlowTracer::exportJsonToFile(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << toJson();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace adapter