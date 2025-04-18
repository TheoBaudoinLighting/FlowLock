#pragma once

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <functional>

namespace adapter {
    class FlowTracer;
    class FlowTask;
}

namespace adapter {
    
class FlowScheduler {
public:
    enum class Strategy {
        FIFO,
        PRIORITY
    };

    FlowScheduler(Strategy strategy = Strategy::PRIORITY);

    void enqueueTask(std::shared_ptr<FlowTask> task);
    std::shared_ptr<FlowTask> dequeueTask();
    bool hasTasks() const;

    void setStrategy(Strategy strategy);
    Strategy getStrategy() const;

    size_t getQueueSize() const;

private:
    struct TaskComparator {
        bool operator()(const std::shared_ptr<FlowTask>& a, const std::shared_ptr<FlowTask>& b) const;
    };

    mutable std::mutex queueMutex;
    std::condition_variable condVar;
    std::priority_queue<std::shared_ptr<FlowTask>, std::vector<std::shared_ptr<FlowTask>>, TaskComparator> taskQueue;
    Strategy currentStrategy;
    std::atomic<bool> stopping{ false };
};

} // namespace adapter