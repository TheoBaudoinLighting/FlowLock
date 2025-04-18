#include "FlowExecution.h"
#include "FlowTask.h"
#include "FlowContext.h"
#include "FlowScheduler.h"
#include "FlowTracer.h"
#include <algorithm>
#include <thread>
#include <exception>

namespace adapter {

    FlowExecution::FlowExecution(FlowScheduler& scheduler)
        : scheduler(scheduler) {
    }

    void FlowExecution::executeTask(std::shared_ptr<FlowTask> task) {
        if (!task) return;

        {
            std::lock_guard<std::mutex> lock(runningTasksMutex);
            runningTasks.push_back(task);
        }

        static std::atomic<uint32_t> nextThreadId{ 0 };
        static std::atomic<uint64_t> nextLogicalTick{ 0 };

        FlowContext context(nextThreadId++, nextLogicalTick++, true);

        std::exception_ptr capturedExcep = nullptr;
        bool taskExecuted = false;

        try {
            try {
                FlowTracer::instance().recordTaskStarted(task, context);
            }
            catch (...) {}

            context.startProfiling("Task Execution");
            task->execute(context);
            taskExecuted = true;
            context.endProfiling();

            executionCounter++;

            try {
                FlowTracer::instance().recordTaskCompleted(task, context);
            }
            catch (...) {}
        }
        catch (const std::exception& e) {
            try {
                FlowTracer::instance().recordTaskFailed(task, context, e.what());
            }
            catch (...) {}

            capturedExcep = std::current_exception();
        }
        catch (...) {
            try {
                FlowTracer::instance().recordTaskFailed(task, context, "Unknown error");
            }
            catch (...) {}

            capturedExcep = std::current_exception();
        }

        {
            std::lock_guard<std::mutex> lock(runningTasksMutex);
            auto it = std::find(runningTasks.begin(), runningTasks.end(), task);
            if (it != runningTasks.end()) {
                runningTasks.erase(it);
            }
        }

        notifyTaskCompleted(task);

        if (capturedExcep && taskExecuted) {
            std::rethrow_exception(capturedExcep);
        }
    }

    void FlowExecution::setTaskCompletionCallback(TaskCompletionCallback callback) {
        completionCallback = callback;
    }

    std::vector<std::shared_ptr<FlowTask>> FlowExecution::getRunningTasks() const {
        std::lock_guard<std::mutex> lock(runningTasksMutex);
        return runningTasks;
    }

    void FlowExecution::notifyTaskCompleted(const std::shared_ptr<FlowTask>& task) {
        if (completionCallback) {
            completionCallback(task);
        }
    }

} // namespace adapter