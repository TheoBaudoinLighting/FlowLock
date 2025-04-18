#include "pch.h"

namespace adapter::Tests {

    class FlowTaskTest : public ::testing::Test {
    protected:
        void SetUp() override {
        }

        void TearDown() override {
        }
    };

    TEST_F(FlowTaskTest, ConstructWithFunctionPriorityAndTimestamp) {
        bool executed = false;
        auto fn = [&executed](FlowContext& ctx) { executed = true; };
        auto timestamp = std::chrono::steady_clock::now();

        FlowTask task(fn, 10, timestamp);

        EXPECT_EQ(task.getPriority(), 10);
        EXPECT_EQ(task.getTimestamp(), timestamp);
        EXPECT_FALSE(executed);

        FlowContext context(1, 1);
        task.execute(context);
        EXPECT_TRUE(executed);
    }

    TEST_F(FlowTaskTest, ManagesTags) {
        auto fn = [](FlowContext&) {};
        FlowTask task(fn);

        EXPECT_FALSE(task.hasTag("render"));
        EXPECT_EQ(task.getTags().size(), 0);

        task.addTag("render");
        EXPECT_TRUE(task.hasTag("render"));
        EXPECT_EQ(task.getTags().size(), 1);

        task.addTag("physics");
        EXPECT_TRUE(task.hasTag("physics"));
        EXPECT_EQ(task.getTags().size(), 2);

        task.addTag("render"); 
        EXPECT_EQ(task.getTags().size(), 2); 
    }

    TEST_F(FlowTaskTest, ExecutesWithContext) {
        uint32_t capturedThreadId = 0;
        uint64_t capturedLogicalTick = 0;

        auto fn = [&capturedThreadId, &capturedLogicalTick](FlowContext& ctx) {
            capturedThreadId = ctx.getThreadId();
            capturedLogicalTick = ctx.getLogicalTick();
            };

        FlowTask task(fn);
        FlowContext context(42, 123);

        task.execute(context);

        EXPECT_EQ(capturedThreadId, 42);
        EXPECT_EQ(capturedLogicalTick, 123);
    }

}  // namespace adapter::Tests