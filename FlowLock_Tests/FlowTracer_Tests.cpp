#include "pch.h"

namespace adapter::Tests {

    class FlowTracerTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().clear();
            FlowTracer::instance().setEnabled(true);
        }

        void TearDown() override {
            FlowTracer::instance().clear();
        }

        std::shared_ptr<FlowTask> createTask(const std::vector<std::string>& tags = {}) {
            auto task = std::make_shared<FlowTask>([](FlowContext&) {});
            for (const auto& tag : tags) {
                task->addTag(tag);
            }
            return task;
        }
    };

    TEST_F(FlowTracerTest, SingletonInstanceExists) {
        auto& instance1 = FlowTracer::instance();
        auto& instance2 = FlowTracer::instance();

        EXPECT_EQ(&instance1, &instance2);
    }

    TEST_F(FlowTracerTest, TracerEnabledByDefault) {
        EXPECT_TRUE(FlowTracer::instance().isEnabled());
    }

    TEST_F(FlowTracerTest, CanDisableAndReEnable) {
        auto& tracer = FlowTracer::instance();

        tracer.setEnabled(false);
        EXPECT_FALSE(tracer.isEnabled());

        tracer.setEnabled(true);
        EXPECT_TRUE(tracer.isEnabled());
    }

    TEST_F(FlowTracerTest, AddEventWithBasicInfo) {
        auto& tracer = FlowTracer::instance();

        tracer.addEvent(
            TraceEvent::Type::TASK_QUEUED,
            "Test event"
        );

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].type, TraceEvent::Type::TASK_QUEUED);
        EXPECT_EQ(events[0].description, "Test event");
        EXPECT_FALSE(events[0].taskId.has_value());
        EXPECT_FALSE(events[0].threadId.has_value());
        EXPECT_TRUE(events[0].tags.empty());
    }

    TEST_F(FlowTracerTest, AddEventWithAllInfo) {
        auto& tracer = FlowTracer::instance();

        tracer.addEvent(
            TraceEvent::Type::TASK_STARTED,
            "Full event",
            42,
            123,
            { "render", "physics" }
        );

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].type, TraceEvent::Type::TASK_STARTED);
        EXPECT_EQ(events[0].description, "Full event");
        EXPECT_TRUE(events[0].taskId.has_value());
        EXPECT_EQ(events[0].taskId.value(), 42);
        EXPECT_TRUE(events[0].threadId.has_value());
        EXPECT_EQ(events[0].threadId.value(), 123);
        EXPECT_EQ(events[0].tags.size(), 2);
        EXPECT_EQ(events[0].tags[0], "render");
        EXPECT_EQ(events[0].tags[1], "physics");
    }

    TEST_F(FlowTracerTest, RecordTaskLifecycle) {
        auto& tracer = FlowTracer::instance();

        auto task = createTask({ "render" });
        FlowContext context(1, 1, true);

        tracer.recordTaskQueued(task);
        tracer.recordTaskStarted(task, context);
        tracer.recordTaskCompleted(task, context);

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 3);
        EXPECT_EQ(events[0].type, TraceEvent::Type::TASK_QUEUED);
        EXPECT_EQ(events[1].type, TraceEvent::Type::TASK_STARTED);
        EXPECT_EQ(events[2].type, TraceEvent::Type::TASK_COMPLETED);
    }

    TEST_F(FlowTracerTest, RecordFailedTask) {
        auto& tracer = FlowTracer::instance();

        auto task = createTask({ "render" });
        FlowContext context(1, 1);

        tracer.recordTaskFailed(task, context, "Test error");

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].type, TraceEvent::Type::TASK_FAILED);
        EXPECT_EQ(events[0].description, "Task failed: Test error");
    }

    TEST_F(FlowTracerTest, RecordSchedulerEmpty) {
        auto& tracer = FlowTracer::instance();

        tracer.recordSchedulerEmpty();

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].type, TraceEvent::Type::SCHEDULER_EMPTY);
        EXPECT_EQ(events[0].description, "Scheduler queue empty");
    }

    TEST_F(FlowTracerTest, RecordConflictDetected) {
        auto& tracer = FlowTracer::instance();

        auto task = createTask({ "render" });

        tracer.recordConflictDetected(task, "Exclusive tag conflict");

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].type, TraceEvent::Type::CONFLICT_DETECTED);
        EXPECT_EQ(events[0].description, "Conflict detected: Exclusive tag conflict");
    }

    TEST_F(FlowTracerTest, ClearEventsWorks) {
        auto& tracer = FlowTracer::instance();

        tracer.addEvent(TraceEvent::Type::TASK_QUEUED, "Test 1");
        tracer.addEvent(TraceEvent::Type::TASK_QUEUED, "Test 2");

        EXPECT_EQ(tracer.getEvents().size(), 2);

        tracer.clear();

        EXPECT_EQ(tracer.getEvents().size(), 0);
    }

    TEST_F(FlowTracerTest, MaxEventsLimitWorks) {
        auto& tracer = FlowTracer::instance();

        tracer.setMaxEvents(5);

        for (int i = 0; i < 10; i++) {
            tracer.addEvent(TraceEvent::Type::TASK_QUEUED, "Event " + std::to_string(i));
        }

        const auto& events = tracer.getEvents();
        EXPECT_EQ(events.size(), 5);

        for (int i = 0; i < 5; i++) {
            EXPECT_EQ(events[i].description, "Event " + std::to_string(i + 5));
        }
    }

}  // namespace adapter::Tests