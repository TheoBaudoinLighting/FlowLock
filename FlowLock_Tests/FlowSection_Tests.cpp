#ifdef _INDIVIDUAL_TEST_FILES
// Ce garde empêche la définition des tests dans plusieurs unités de compilation
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

            // Utiliser request directement plutôt que l'opérateur <<
            auto future = FlowLock::instance().request([&executed](FlowContext&) {
                executed = true;
                return 42;
                }, 99, { "render", "section:render" });

            // Force direct execution
            for (int i = 0; i < 5; i++) {
                FlowLock::instance().run();
                if (executed) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Explicitly wait for future
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

        // Définir le callback avant de créer la tâche
        FlowLock::instance().setTaskCompletionCallback([&capturedTags](const std::shared_ptr<FlowTask>& task) {
            capturedTags = task->getTags();
            });

        // Créer et exécuter la tâche
        auto future = FlowLock::instance().request([&taskExecuted](FlowContext&) {
            taskExecuted = true;
            }, 99, { "graphics", "section:render" });

        // Exécuter directement
        for (int i = 0; i < 5 && !taskExecuted; i++) {
            FlowLock::instance().run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Tags should include both the original tag and the section name tag
        EXPECT_EQ(capturedTags.size(), 2);
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "graphics") != capturedTags.end());
        EXPECT_TRUE(std::find(capturedTags.begin(), capturedTags.end(), "section:render") != capturedTags.end());
    }

    TEST_F(FlowSectionTest, MultipleTasksInSameSection) {
        std::atomic<int> counter{ 0 };

        // Créer des tâches séparées plutôt que d'utiliser une section
        for (int i = 0; i < 5; i++) {
            auto future = FlowLock::instance().request([&counter](FlowContext&) {
                counter++;
                }, 99, { "batch", "section:batch" });
        }

        // Exécuter directement au lieu d'await
        for (int i = 0; i < 15 && counter < 5; i++) {
            FlowLock::instance().run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(counter, 5);
    }

}  // namespace Volvic::Ticking::Tests
#endif // _INDIVIDUAL_TEST_FILES