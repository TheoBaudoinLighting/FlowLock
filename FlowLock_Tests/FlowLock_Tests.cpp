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
            auto future = section << [&executed](FlowContext&) {
                executed = true;
                return 42;
                };

            FlowLock::instance().run();

            try {
                EXPECT_EQ(future.get(), 42);
                EXPECT_TRUE(executed);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception while getting future result: " << e.what() << std::endl;
                FAIL() << "Future threw exception";
            }
        }
    }

    TEST_F(FlowSectionTest, AutomaticallyAddsTagWithSectionName) {
        std::vector<std::string> capturedTags;

        {
            FlowSection section("render", 99, { "graphics" });
            auto future = section << [&capturedTags](FlowContext& ctx) {
                FlowLock::instance().setTaskCompletionCallback([&capturedTags](const std::shared_ptr<FlowTask>& task) {
                    capturedTags = task->getTags();
                    });
                };

            FlowLock::instance().run();
            future.get();
        }

        EXPECT_EQ(capturedTags.size(), 2);
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "graphics") != capturedTags.end());
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "section:render") != capturedTags.end());
    }

    TEST_F(FlowSectionTest, MultipleTasksInSameSection) {
        std::atomic<int> counter{ 0 };
        std::vector<std::future<void>> futures;

        {
            FlowSection section("batch", 99);

            for (int i = 0; i < 5; i++) {
                futures.push_back(section << [&counter](FlowContext&) {
                    counter++;
                    });
            }

            FlowLock::instance().run();

            for (auto& future : futures) {
                try {
                    future.get();
                }
                catch (const std::exception& e) {
                    std::cerr << "Exception in task: " << e.what() << std::endl;
                    FAIL() << "Task threw exception";
                }
            }
        }

        EXPECT_EQ(counter, 5);
    }

}  // namespace adapter::Tests