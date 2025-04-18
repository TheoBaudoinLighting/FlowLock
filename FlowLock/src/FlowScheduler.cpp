#include "FlowScheduler.h"
#include "FlowTask.h"
#include "FlowTracer.h"

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
        }

        condVar.notify_one();
    }

    std::shared_ptr<FlowTask> FlowScheduler::dequeueTask() {
        std::unique_lock<std::mutex> lock(queueMutex);

        if (taskQueue.empty()) {
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