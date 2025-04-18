#include "pch.h"

namespace Volvic::Ticking::Tests {

    class MockScheduler : public FlowScheduler {
    public:
        MOCK_METHOD1(enqueueTask, void(std::shared_ptr<FlowTask>));
    };

    class FlowExecutionTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Disable FlowTracer during tests to avoid side effects
            FlowTracer::instance().setEnabled(false);
            mockScheduler = std::make_unique<MockScheduler>();
            execution = std::make_unique<FlowExecution>(*mockScheduler);
        }

        void TearDown() override {
            // Re-enable FlowTracer after tests
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

        // Execute in another thread
        std::thread executor([this, longRunningTask]() {
            execution->executeTask(longRunningTask);
            });

        // Give it time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Check running tasks
        auto runningTasks = execution->getRunningTasks();
        EXPECT_EQ(runningTasks.size(), 1);
        EXPECT_EQ(runningTasks[0], longRunningTask);

        // Signal task to complete
        keepRunning = false;
        executor.join();

        // Check running tasks are empty
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

        // Nous nous attendons maintenant à ce que l'exécution gère l'exception en interne
        // et ne la propage pas, mais qu'elle nettoie quand même correctement
        execution->executeTask(throwingTask);

        // Le callback doit quand même être appelé
        EXPECT_TRUE(callbackCalled);

        // La tâche ne doit pas rester dans la liste des tâches en cours
        EXPECT_TRUE(execution->getRunningTasks().empty());
    }

}  // namespace Volvic::Ticking::Tests