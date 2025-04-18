#ifdef _INDIVIDUAL_TEST_FILES
#include "pch.h"

namespace adapter::Tests {

    class FlowSectionTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().setEnabled(false);
        }

        void TearDown() override {
            FlowTracer::instance().setEnabled(true);
        }
    };

    TEST_F(FlowSectionTest, CreateSectionWithNameAndPriority) {
        FlowSection section("render", 99);
    }

    TEST_F(FlowSectionTest, QueueTasksWithOperator) {
        bool executed = false;

        {
            FlowSection section("render", 99);

            auto future = FlowLock::instance().request([&executed](FlowContext&) {
                executed = true;
                return 42;
                }, 99, { "render", "section:render" });

            for (int i = 0; i < 5; i++) {
                FlowLock::instance().run();
                if (executed) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            try {
                auto status = future.wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    EXPECT_EQ(future.get(), 42);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception while getting future result: " << e.what() << std::endl;
                FAIL() << "Future threw exception";
            }

            EXPECT_TRUE(executed);
        }
    }

    TEST_F(FlowSectionTest, AutomaticallyAddsTagWithSectionName) {
        std::vector<std::string> capturedTags;
        bool taskExecuted = false;

        FlowLock::instance().setTaskCompletionCallback([&capturedTags](const std::shared_ptr<FlowTask>& task) {
            capturedTags = task->getTags();
            });

        auto future = FlowLock::instance().request([&taskExecuted](FlowContext&) {
            taskExecuted = true;
            }, 99, { "graphics", "section:render" });

        for (int i = 0; i < 5 && !taskExecuted; i++) {
            FlowLock::instance().run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(capturedTags.size(), 2);
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "graphics") != capturedTags.end());
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "section:render") != capturedTags.end());
    }

    TEST_F(FlowSectionTest, MultipleTasksInSameSection) {
        std::atomic<int> counter{ 0 };

        for (int i = 0; i < 5; i++) {
            auto future = FlowLock::instance().request([&counter](FlowContext&) {
                counter++;
                }, 99, { "batch", "section:batch" });
        }

        for (int i = 0; i < 15 && counter < 5; i++) {
            FlowLock::instance().run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(counter, 5);
    }

}  // namespace adapter::Tests
#endif // _INDIVIDUAL_TEST_FILES