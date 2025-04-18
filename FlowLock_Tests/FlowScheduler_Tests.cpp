#include "pch.h"

namespace adapter::Tests {

    class FlowSchedulerTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().setEnabled(false);
        }

        void TearDown() override {
            FlowTracer::instance().setEnabled(true);
        }
    };

    TEST_F(FlowSchedulerTest, CreateWithDefaultStrategy) {
        FlowScheduler scheduler;
        EXPECT_EQ(scheduler.getStrategy(), FlowScheduler::Strategy::PRIORITY);
        EXPECT_EQ(scheduler.getQueueSize(), 0);
        EXPECT_FALSE(scheduler.hasTasks());
    }

    TEST_F(FlowSchedulerTest, EnqueueAndDequeueTask) {
        FlowScheduler scheduler;

        auto task = std::make_shared<FlowTask>([](FlowContext&) {});
        scheduler.enqueueTask(task);

        EXPECT_EQ(scheduler.getQueueSize(), 1);
        EXPECT_TRUE(scheduler.hasTasks());

        auto dequeuedTask = scheduler.dequeueTask();
        EXPECT_EQ(dequeuedTask, task);
        EXPECT_EQ(scheduler.getQueueSize(), 0);
        EXPECT_FALSE(scheduler.hasTasks());
    }

    TEST_F(FlowSchedulerTest, PriorityOrderingWorks) {
        FlowScheduler scheduler;

        auto lowPriorityTask = std::make_shared<FlowTask>([](FlowContext&) {}, 10);
        auto highPriorityTask = std::make_shared<FlowTask>([](FlowContext&) {}, 20);

        scheduler.enqueueTask(lowPriorityTask);
        scheduler.enqueueTask(highPriorityTask);

        EXPECT_EQ(scheduler.getQueueSize(), 2);

        auto firstTask = scheduler.dequeueTask();
        EXPECT_EQ(firstTask, highPriorityTask);

        auto secondTask = scheduler.dequeueTask();
        EXPECT_EQ(secondTask, lowPriorityTask);
    }

    TEST_F(FlowSchedulerTest, TimeBasedOrderingWorks) {
        FlowScheduler scheduler;

        auto timestamp1 = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto timestamp2 = std::chrono::steady_clock::now();

        auto task1 = std::make_shared<FlowTask>([](FlowContext&) {}, 10, timestamp1);
        auto task2 = std::make_shared<FlowTask>([](FlowContext&) {}, 10, timestamp2);

        scheduler.enqueueTask(task1);
        scheduler.enqueueTask(task2);

        auto firstTask = scheduler.dequeueTask();
        EXPECT_EQ(firstTask, task1);

        auto secondTask = scheduler.dequeueTask();
        EXPECT_EQ(secondTask, task2);
    }

    TEST_F(FlowSchedulerTest, DequeueEmptyQueueReturnsNullptr) {
        FlowScheduler scheduler;

        EXPECT_FALSE(scheduler.hasTasks());
        auto task = scheduler.dequeueTask();
        EXPECT_EQ(task, nullptr);
    }

}  // namespace adapter::Tests