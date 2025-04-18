#include "pch.h"

namespace adapter::Tests {

    class MockScheduler : public FlowScheduler {
    public:
        MOCK_METHOD1(enqueueTask, void(std::shared_ptr<FlowTask>));
    };

    class FlowExecutionTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().setEnabled(false);
            mockScheduler = std::make_unique<MockScheduler>();
            execution = std::make_unique<FlowExecution>(*mockScheduler);
        }

        void TearDown() override {
            FlowTracer::instance().setEnabled(true);
            execution.reset();
            mockScheduler.reset();
        }

        std::unique_ptr<MockScheduler> mockScheduler;
        std::unique_ptr<FlowExecution> execution;
    };

    TEST_F(FlowExecutionTest, ExecutesTask) {
        bool executed = false;
        auto task = std::make_shared<FlowTask>([&executed](FlowContext&) {
            executed = true;
            });

        execution->executeTask(task);

        EXPECT_TRUE(executed);
    }

    TEST_F(FlowExecutionTest, TracksRunningTasks) {
        std::atomic<bool> keepRunning{ true };

        auto longRunningTask = std::make_shared<FlowTask>([&keepRunning](FlowContext&) {
            while (keepRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            });

        std::thread executor([this, longRunningTask]() {
            execution->executeTask(longRunningTask);
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        auto runningTasks = execution->getRunningTasks();
        EXPECT_EQ(runningTasks.size(), 1);
        EXPECT_EQ(runningTasks[0], longRunningTask);

        keepRunning = false;
        executor.join();

        runningTasks = execution->getRunningTasks();
        EXPECT_TRUE(runningTasks.empty());
    }

    TEST_F(FlowExecutionTest, NotifiesOnTaskCompletion) {
        bool callbackCalled = false;
        std::shared_ptr<FlowTask> callbackTask;

        execution->setTaskCompletionCallback([&callbackCalled, &callbackTask](const std::shared_ptr<FlowTask>& task) {
            callbackCalled = true;
            callbackTask = task;
            });

        auto task = std::make_shared<FlowTask>([](FlowContext&) {});

        execution->executeTask(task);

        EXPECT_TRUE(callbackCalled);
        EXPECT_EQ(callbackTask, task);
    }

    TEST_F(FlowExecutionTest, HandlesExceptions) {
        bool callbackCalled = false;

        execution->setTaskCompletionCallback([&callbackCalled](const std::shared_ptr<FlowTask>&) {
            callbackCalled = true;
            });

        auto throwingTask = std::make_shared<FlowTask>([](FlowContext&) {
            throw std::runtime_error("Test exception");
            });

        execution->executeTask(throwingTask);

        EXPECT_TRUE(callbackCalled);

        EXPECT_TRUE(execution->getRunningTasks().empty());
    }

}  // namespace adapter::Tests