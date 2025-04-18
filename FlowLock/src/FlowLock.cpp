#include "FlowLock.h"
#include "FlowTask.h"
#include "FlowScheduler.h"
#include "FlowExecution.h"
#include "ConflictResolver.h"
#include "ThreadPool.h"
#include "FlowTracer.h"
#include <thread>
#include <chrono>
#include <iostream>

namespace Volvic::Ticking {

    FlowLock& FlowLock::instance() {
        static FlowLock instance;
        return instance;
    }

    FlowLock::FlowLock()
        : scheduler(std::make_unique<FlowScheduler>(FlowScheduler::Strategy::PRIORITY)),
        execution(std::make_unique<FlowExecution>(*scheduler)),
        conflictResolver(std::make_unique<ConflictResolver>()),
        threadPool(std::make_unique<ThreadPool>()) {

        std::cerr << "FlowLock constructor called" << std::endl;

        execution->setTaskCompletionCallback(
            [this](const std::shared_ptr<FlowTask>& task) {
                onTaskCompleted(task);
            }
        );
    }

    FlowLock::~FlowLock() {
        await();
    }

    void FlowLock::await() {
        std::unique_lock<std::mutex> lock(processMutex);

        const auto timeout = std::chrono::seconds(5);
        auto waitResult = scheduleCondVar.wait_for(lock, timeout, [this]() {
            return !scheduler->hasTasks() && execution->getRunningTasks().empty();
            });

        if (!waitResult) {
            std::cerr << "FlowLock::await() - Timeout after " << timeout.count()
                << " seconds. Tasks remaining: " << scheduler->getQueueSize()
                << ", Tasks in progress: " << execution->getRunningTasks().size() << std::endl;
        }
    }

    void FlowLock::run() {
        const int maxIterations = 100;
        int iteration = 0;
        bool progressMade = false;

        while (scheduler->hasTasks() && iteration < maxIterations) {
            iteration++;

            auto task = scheduler->dequeueTask();
            if (!task) continue;

            std::vector<std::shared_ptr<FlowTask>> currentRunningTasks;
            {
                currentRunningTasks = execution->getRunningTasks();
            }

            if (conflictResolver->canExecute(task, currentRunningTasks)) {
                execution->executeTask(task);
                progressMade = true;
            }
            else {
                scheduler->enqueueTask(task);

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        if (iteration >= maxIterations) {
            std::cerr << "FlowLock::run() - Reached maximum iterations ("
                << maxIterations << "). Tasks remaining: "
                << scheduler->getQueueSize() << std::endl;
        }
    }

    void FlowLock::setThreadPoolSize(size_t threads) {
        threadPool->resize(threads);
    }

    void FlowLock::processNextTask() {
        std::shared_ptr<FlowTask> task;
        {
            std::lock_guard<std::mutex> lock(processMutex);
            task = scheduler->dequeueTask();
        }

        if (!task) return;

        std::vector<std::shared_ptr<FlowTask>> currentRunningTasks;
        {
            currentRunningTasks = execution->getRunningTasks();
        }

        if (conflictResolver->canExecute(task, currentRunningTasks)) {
            execution->executeTask(task);
        }
        else {
            std::lock_guard<std::mutex> lock(processMutex);
            scheduler->enqueueTask(task);
        }
    }

    void FlowLock::onTaskCompleted(const std::shared_ptr<FlowTask>& task) {
        if (userCompletionCallback) {
            userCompletionCallback(task);
        }

        processNextTask();

        if (!scheduler->hasTasks() && execution->getRunningTasks().empty()) {
            std::lock_guard<std::mutex> lock(processMutex);
            allTasksCompleted = true;
            scheduleCondVar.notify_all();
        }
    }

    void FlowLock::setTaskCompletionCallback(TaskCompletionCallback callback) {
        userCompletionCallback = callback;
    }

} // namespace Volvic::Ticking