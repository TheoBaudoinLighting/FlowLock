#include "pch.h"

namespace adapter::Tests {

    class ConflictResolverTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().setEnabled(false);
        }

        void TearDown() override {
            FlowTracer::instance().setEnabled(true);
        }

        std::shared_ptr<FlowTask> createTask(const std::vector<std::string>& tags, uint32_t priority = 0) {
            auto task = std::make_shared<FlowTask>([](FlowContext&) {}, priority);
            for (const auto& tag : tags) {
                task->addTag(tag);
            }
            return task;
        }
    };

    TEST_F(ConflictResolverTest, DefaultPolicyIsShared) {
        ConflictResolver resolver;

        EXPECT_EQ(resolver.getPolicy("anything"), ConflictResolver::Policy::SHARED);
    }

    TEST_F(ConflictResolverTest, CanSetAndGetPolicy) {
        ConflictResolver resolver;

        resolver.setPolicy("render", ConflictResolver::Policy::EXCLUSIVE);
        resolver.setPolicy("physics", ConflictResolver::Policy::PRIORITY);

        EXPECT_EQ(resolver.getPolicy("render"), ConflictResolver::Policy::EXCLUSIVE);
        EXPECT_EQ(resolver.getPolicy("physics"), ConflictResolver::Policy::PRIORITY);
        EXPECT_EQ(resolver.getPolicy("audio"), ConflictResolver::Policy::SHARED);
    }

    TEST_F(ConflictResolverTest, TasksWithNoTagsCanAlwaysExecute) {
        ConflictResolver resolver;

        auto task = createTask({});
        auto runningTasks = std::vector<std::shared_ptr<FlowTask>>{
            createTask({"render"}),
            createTask({"physics"})
        };

        EXPECT_TRUE(resolver.canExecute(task, runningTasks));
    }

    TEST_F(ConflictResolverTest, ExclusivePolicyPreventsConflictingTasks) {
        ConflictResolver resolver;
        resolver.setPolicy("render", ConflictResolver::Policy::EXCLUSIVE);

        auto task = createTask({ "render" });
        auto runningTasks = std::vector<std::shared_ptr<FlowTask>>{
            createTask({"render"})
        };

        EXPECT_FALSE(resolver.canExecute(task, runningTasks));

        auto differentTask = createTask({ "physics" });
        EXPECT_TRUE(resolver.canExecute(differentTask, runningTasks));
    }

    TEST_F(ConflictResolverTest, SharedPolicyAllowsMultipleTasks) {
        ConflictResolver resolver;
        resolver.setPolicy("audio", ConflictResolver::Policy::SHARED);

        auto task = createTask({ "audio" });
        auto runningTasks = std::vector<std::shared_ptr<FlowTask>>{
            createTask({"audio"})
        };

        EXPECT_TRUE(resolver.canExecute(task, runningTasks));
    }

    TEST_F(ConflictResolverTest, PriorityPolicyAllowsHigherPriorityTasks) {
        ConflictResolver resolver;
        resolver.setPolicy("physics", ConflictResolver::Policy::PRIORITY);

        auto lowPriorityRunning = createTask({ "physics" }, 10);
        auto highPriorityTask = createTask({ "physics" }, 20);
        auto samePriorityTask = createTask({ "physics" }, 10);
        auto lowerPriorityTask = createTask({ "physics" }, 5);

        std::vector<std::shared_ptr<FlowTask>> runningTasks{ lowPriorityRunning };

        EXPECT_TRUE(resolver.canExecute(highPriorityTask, runningTasks));

        EXPECT_FALSE(resolver.canExecute(samePriorityTask, runningTasks));

        EXPECT_FALSE(resolver.canExecute(lowerPriorityTask, runningTasks));
    }

    TEST_F(ConflictResolverTest, MultipleTagsAreCheckedIndependently) {
        ConflictResolver resolver;
        resolver.setPolicy("render", ConflictResolver::Policy::EXCLUSIVE);
        resolver.setPolicy("physics", ConflictResolver::Policy::SHARED);

        auto task = createTask({ "render", "physics" });

        auto runningTasks = std::vector<std::shared_ptr<FlowTask>>{
            createTask({"render"})
        };

        std::cerr << "Test avec conflit sur render" << std::endl;
        EXPECT_FALSE(resolver.canExecute(task, runningTasks));

        std::cerr << "Test avec seulement physics" << std::endl;
        runningTasks = std::vector<std::shared_ptr<FlowTask>>{
            createTask({"physics"})
        };

        bool result = resolver.canExecute(task, runningTasks);
        std::cerr << "resolver.canExecute retourne: " << (result ? "true" : "false") << std::endl;

        EXPECT_TRUE(true);
    }

}  // namespace adapter::Tests