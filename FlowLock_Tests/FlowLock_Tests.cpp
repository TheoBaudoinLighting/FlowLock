#include "pch.h"

namespace Volvic::Ticking::Tests {

    class FlowSectionTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Disable FlowTracer during tests to avoid side effects
            FlowTracer::instance().setEnabled(false);
        }

        void TearDown() override {
            // Re-enable FlowTracer after tests
            FlowTracer::instance().setEnabled(true);
        }
    };

    TEST_F(FlowSectionTest, CreateSectionWithNameAndPriority) {
        FlowSection section("render", 99);

        // No explicit assertions, but should not crash
    }

    TEST_F(FlowSectionTest, QueueTasksWithOperator) {
        bool executed = false;

        {
            FlowSection section("render", 99);
            auto future = section << [&executed](FlowContext&) {
                executed = true;
                return 42;
                };

            // Force direct execution instead of async
            FlowLock::instance().run();

            // Explicitly wait for future
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
                // We need to capture the task tags indirectly
                FlowLock::instance().setTaskCompletionCallback([&capturedTags](const std::shared_ptr<FlowTask>& task) {
                    capturedTags = task->getTags();
                    });
                };

            FlowLock::instance().run();
            future.get();
        }

        // Tags should include both the original tag and the section name tag
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

            // Force execution and explicitly wait for tasks
            FlowLock::instance().run();

            // Explicitly wait for all futures
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

}  // namespace Volvic::Ticking::Tests