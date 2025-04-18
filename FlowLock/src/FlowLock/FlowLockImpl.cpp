#include "FlowLock/FlowLockImpl.h"
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Scheduler/FlowScheduler.h"
#include "FlowLock/Execution/FlowExecution.h"
#include "FlowLock/Context/FlowContext.h"
#include "FlowLock/Utils/ThreadPool.h"
#include "FlowLock/Utils/FlowTracer.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <unordered_map>

namespace adapter {

FlowLockImpl& FlowLockImpl::instance() {
    static FlowLockImpl instance;
    return instance;
}

FlowLockImpl::FlowLockImpl()
    : scheduler(std::make_unique<FlowScheduler>(FlowScheduler::Strategy::PRIORITY)),
    execution(std::make_unique<FlowExecution>(*scheduler)),
    conflictResolver(std::make_unique<ConflictResolver>()),
    threadPool(std::make_unique<ThreadPool>()),
    antiStarvationLimit(10) {

    int numProcessors = std::max(2, static_cast<int>(std::thread::hardware_concurrency() / 2));
    
    for (int i = 0; i < numProcessors; i++) {
        threadPool->enqueue([this, i]() {
            while (!stopping) {
                processNextTask();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    execution->setTaskCompletionCallback(
        [this](const std::shared_ptr<FlowTask>& task) {
            onTaskCompleted(task);
        }
    );
    
    // Anti-starvation setup - utiliser une approche différente sans modifier canExecute
    std::unordered_map<void*, size_t>* reEnqueueCounts = new std::unordered_map<void*, size_t>();
    
    // Pas besoin de modifier conflictResolver ici
}

FlowLockImpl::~FlowLockImpl() {
    shutdown();
}

bool FlowLockImpl::await(std::chrono::milliseconds timeout) {
    auto endTime = std::chrono::steady_clock::now() + timeout;
    
    while (std::chrono::steady_clock::now() < endTime) {
        if (!scheduler->hasTasks() && execution->getRunningTasks().empty()) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return !scheduler->hasTasks() && execution->getRunningTasks().empty();
}

void FlowLockImpl::shutdown() {
    stopping = true;
    threadPool->waitForTasks();
    await();
}

void FlowLockImpl::run() {
    const int maxIterations = 100;
    int iteration = 0;
    int tasksProcessed = 0;

    while (scheduler->hasTasks() && iteration < maxIterations) {
        iteration++;

        auto task = scheduler->dequeueTask();
        if (!task) continue;

        std::vector<std::shared_ptr<FlowTask>> currentRunningTasks = execution->getRunningTasks();

        if (conflictResolver->canExecute(task, currentRunningTasks)) {
            try {
                execution->executeTask(task);
                tasksProcessed++;
            } catch (...) {
                failedTaskCount++;
            }
        }
        else {
            scheduler->enqueueTask(task);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void FlowLockImpl::setThreadPoolSize(size_t threads) {
    threadPool->resize(threads);
}

void FlowLockImpl::processNextTask() {
    std::shared_ptr<FlowTask> task;
    {
        std::lock_guard<std::mutex> lock(processMutex);
        task = scheduler->dequeueTask();
    }

    if (!task) return;

    std::vector<std::shared_ptr<FlowTask>> currentRunningTasks = execution->getRunningTasks();

    // Implémenter anti-starvation ici
    static std::unordered_map<void*, size_t> taskReEnqueueCount;
    
    bool canRun = conflictResolver->canExecute(task, currentRunningTasks);
    if (!canRun) {
        void* taskPtr = task.get();
        auto& count = taskReEnqueueCount[taskPtr];
        count++;
        reEnqueuedTaskCount++;
        
        // Si réenfilé trop souvent, on force l'exécution
        if (count > antiStarvationLimit) {
            canRun = true;
            // On pourrait appeler FlowTracer ici
        }
    }
    
    if (canRun) {
        try {
            execution->executeTask(task);
            
            // Réinitialiser le compteur
            void* taskPtr = task.get();
            taskReEnqueueCount.erase(taskPtr);
        } catch (...) {
            failedTaskCount++;
        }
    }
    else {
        std::lock_guard<std::mutex> lock(processMutex);
        scheduler->enqueueTask(task);
        task->incrementReenqueueCount();
    }
}

void FlowLockImpl::onTaskCompleted(const std::shared_ptr<FlowTask>& task) {
    if (!task) {
        return;
    }

    if (userCompletionCallback) {
        try {
            userCompletionCallback(task);
        } catch (...) {
            // Log error
        }
    }

    completedTaskCount++;

    try {
        processNextTask();
    } catch (...) {
        // Log error
    }

    bool allCompleted = !scheduler->hasTasks() && execution->getRunningTasks().empty();

    if (allCompleted) {
        std::lock_guard<std::mutex> lock(processMutex);
        allTasksCompleted = true;
        scheduleCondVar.notify_all();
    }
}

void FlowLockImpl::setTaskCompletionCallback(TaskCompletionCallback callback) {
    userCompletionCallback = callback;
}

void FlowLockImpl::setPolicy(const std::string& tag, ConflictResolver::Policy policy) {
    conflictResolver->setPolicy(tag, policy);
}

void FlowLockImpl::setDefaultPolicy(ConflictResolver::Policy policy) {
    setPolicy("default", policy);
}

FlowLockImpl::Stats FlowLockImpl::stats() const {
    return {
        scheduler->getQueueSize(),
        execution->getRunningTasks().size(),
        completedTaskCount.load(),
        failedTaskCount.load(),
        reEnqueuedTaskCount.load()
    };
}

std::string FlowLockImpl::debugDump() const {
    std::stringstream ss;
    
    auto currentStats = stats();
    
    ss << "FlowLock Debug Dump:\n";
    ss << "==================\n";
    ss << "Queued tasks: " << currentStats.queuedTaskCount << "\n";
    ss << "Running tasks: " << currentStats.runningTaskCount << "\n";
    ss << "Completed tasks: " << currentStats.completedTaskCount << "\n";
    ss << "Failed tasks: " << currentStats.failedTaskCount << "\n";
    ss << "Re-enqueued tasks: " << currentStats.reEnqueuedCount << "\n";
    ss << "Anti-starvation limit: " << antiStarvationLimit << "\n";
    ss << "==================\n";
    
    ss << "Running Tasks:\n";
    for (const auto& task : execution->getRunningTasks()) {
        ss << "- Priority: " << task->getPriority() << ", Tags: ";
        for (const auto& tag : task->getTags()) {
            ss << tag << " ";
        }
        ss << "\n";
    }
    
    return ss.str();
}

void FlowLockImpl::setAntiStarvationLimit(size_t limit) {
    antiStarvationLimit = limit;
}

size_t FlowLockImpl::getAntiStarvationLimit() const {
    return antiStarvationLimit;
}

} // namespace adapter