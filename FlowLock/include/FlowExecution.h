#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace Volvic::Ticking {

    class FlowTask;
    class FlowContext;
    class FlowScheduler;

    class FlowExecution {
    public:
        using TaskCompletionCallback = std::function<void(const std::shared_ptr<FlowTask>&)>;

        FlowExecution(FlowScheduler& scheduler);

        void executeTask(std::shared_ptr<FlowTask> task);
        void setTaskCompletionCallback(TaskCompletionCallback callback);

        std::vector<std::shared_ptr<FlowTask>> getRunningTasks() const;

        std::atomic<int>& getExecutionCounter() { return executionCounter; }

    private:
        FlowScheduler& scheduler;
        TaskCompletionCallback completionCallback;
        mutable std::mutex runningTasksMutex;
        std::vector<std::shared_ptr<FlowTask>> runningTasks;
        std::atomic<int> executionCounter{ 0 };

        void notifyTaskCompleted(const std::shared_ptr<FlowTask>& task);
    };

} // namespace Volvic::Ticking