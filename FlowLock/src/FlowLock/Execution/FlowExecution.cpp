#include "FlowLock/Execution/FlowExecution.h"
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Context/FlowContext.h"
#include "FlowLock/Scheduler/FlowScheduler.h"
#include "FlowLock/Utils/FlowTracer.h"
#include <algorithm>
#include <thread>
#include <exception>
#include <iostream>

namespace adapter {

    FlowExecution::FlowExecution(FlowScheduler& scheduler)
        : scheduler(scheduler) {
    }

    void FlowExecution::executeTask(std::shared_ptr<FlowTask> task) {
        std::cerr << "[DEBUG] FlowExecution::executeTask CALLED\n";
        
        if (!task) return;

        bool taskRegistered = false;
        try {
            {
                std::lock_guard<std::mutex> lock(runningTasksMutex);
                runningTasks.push_back(task);
                taskRegistered = true;
                std::cerr << "Task execution started - running tasks: " << runningTasks.size() << std::endl;
            }

            static std::atomic<uint32_t> nextThreadId{ 0 };
            static std::atomic<uint64_t> nextLogicalTick{ 0 };

            FlowContext context(nextThreadId++, nextLogicalTick++, true);

            std::exception_ptr capturedExcep = nullptr;
            bool taskExecuted = false;

            try {
                try {
                    FlowTracer::instance().recordTaskStarted(task, context);
                } catch (...) {}

                context.startProfiling("Task Execution");
                task->execute(context);
                taskExecuted = true;
                context.endProfiling();

                executionCounter++;
                std::cerr << "Task executed successfully - counter: " << executionCounter << std::endl;

                try {
                    FlowTracer::instance().recordTaskCompleted(task, context);
                } catch (...) {}
            }
            catch (const std::exception& e) {
                std::cerr << "Task execution failed with exception: " << e.what() << std::endl;
                try {
                    FlowTracer::instance().recordTaskFailed(task, context, e.what());
                }
                catch (...) {}

                capturedExcep = std::current_exception();
            }
            catch (...) {
                std::cerr << "Task execution failed with unknown exception" << std::endl;
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
                    std::cerr << "Task removed from running tasks - remaining: " << runningTasks.size() << std::endl;
                } else {
                    std::cerr << "WARNING: Task not found in running tasks list!" << std::endl;
                }
            }

            notifyTaskCompleted(task);

            if (capturedExcep && taskExecuted) {
                std::rethrow_exception(capturedExcep);
            }
        }
        catch (...) {
            if (taskRegistered) {
                std::lock_guard<std::mutex> lock(runningTasksMutex);
                auto it = std::find(runningTasks.begin(), runningTasks.end(), task);
                if (it != runningTasks.end()) {
                    runningTasks.erase(it);
                    std::cerr << "Emergency task cleanup from running tasks - remaining: " 
                            << runningTasks.size() << std::endl;
                }
            }
            throw; 
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