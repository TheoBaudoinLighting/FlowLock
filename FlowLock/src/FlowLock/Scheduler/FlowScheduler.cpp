#include "FlowLock/Scheduler/FlowScheduler.h"
#include "FlowLock/Scheduler/FlowTask.h"
#include "FlowLock/Utils/FlowTracer.h"
#include <iostream>

namespace adapter {

    bool FlowScheduler::TaskComparator::operator()(const std::shared_ptr<FlowTask>& a, const std::shared_ptr<FlowTask>& b) const {
        if (a->getPriority() != b->getPriority()) {
            return a->getPriority() < b->getPriority();  // Higher number = higher priority
        }

        return a->getTimestamp() > b->getTimestamp();  // Earlier timestamp = higher priority
    }

    FlowScheduler::FlowScheduler(Strategy strategy)
        : currentStrategy(strategy) {
    }

    void FlowScheduler::enqueueTask(std::shared_ptr<FlowTask> task) {
        if (!task) return;

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            taskQueue.push(task);
            std::cerr << "Task enqueued successfully - queue size: " << taskQueue.size() << std::endl;
        }

        condVar.notify_one();
    }

    std::shared_ptr<FlowTask> FlowScheduler::dequeueTask() {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        bool taskAvailable = condVar.wait_for(lock, std::chrono::milliseconds(10), 
            [this] { return !taskQueue.empty() || stopping; });
        
        if (stopping) return nullptr;
        
        if (!taskAvailable || taskQueue.empty()) {
            static std::atomic<uint64_t> emptyCount{ 0 };
            uint64_t currentEmptyCount = ++emptyCount;

            if (currentEmptyCount % 100 == 0) {
                fprintf(stderr, "Scheduler empty (count: %llu)\n", static_cast<unsigned long long>(currentEmptyCount));
            }

            try {
                FlowTracer::instance().recordSchedulerEmpty();
            }
            catch (...) {
            }

            return nullptr;
        }

        auto task = taskQueue.top();
        taskQueue.pop();
        std::cerr << "Task dequeued - remaining queue size: " << taskQueue.size() << std::endl;
        return task;
    }

    bool FlowScheduler::hasTasks() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return !taskQueue.empty();
    }

    void FlowScheduler::setStrategy(Strategy strategy) {
        std::lock_guard<std::mutex> lock(queueMutex);
        currentStrategy = strategy;
    }

    FlowScheduler::Strategy FlowScheduler::getStrategy() const {
        return currentStrategy;
    }

    size_t FlowScheduler::getQueueSize() const {
        std::lock_guard<std::mutex> lock(queueMutex);
        return taskQueue.size();
    }

} // namespace adapter