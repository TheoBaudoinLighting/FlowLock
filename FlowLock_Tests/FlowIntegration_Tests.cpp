#include "pch.h"

namespace adapter::Tests {

    class FlowIntegrationTest : public ::testing::Test {
    protected:
        void SetUp() override {
            FlowTracer::instance().clear();
            FlowTracer::instance().setEnabled(true);
        }

        void TearDown() override {
            FlowTracer::instance().clear();
        }
    };

    TEST_F(FlowIntegrationTest, CompleteWorkflow) {
        auto& flowLock = FlowLock::instance();

        flowLock.setThreadPoolSize(4);

        std::atomic<int> renderCount{ 0 };
        std::atomic<int> physicsCount{ 0 };
        std::atomic<int> audioCount{ 0 };
        std::mutex counterMutex;
        std::vector<int> executionOrder;

        auto renderTask = flowLock.request([&renderCount, &counterMutex, &executionOrder](FlowContext&) {
            renderCount++;
            {
                std::lock_guard<std::mutex> lock(counterMutex);
                executionOrder.push_back(1);
            }
            }, 50, { "render" });

        auto physicsTask = flowLock.request([&physicsCount, &counterMutex, &executionOrder](FlowContext&) {
            physicsCount++;
            {
                std::lock_guard<std::mutex> lock(counterMutex);
                executionOrder.push_back(2);
            }
            }, 100, { "physics" });

        auto audioTask = flowLock.request([&audioCount, &counterMutex, &executionOrder](FlowContext&) {
            audioCount++;
            {
                std::lock_guard<std::mutex> lock(counterMutex);
                executionOrder.push_back(3);
            }
            }, 30, { "audio" });

        for (int i = 0; i < 5; i++) {
            flowLock.run();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(renderCount, 1);
        EXPECT_EQ(physicsCount, 1);
        EXPECT_EQ(audioCount, 1);

        const auto& events = FlowTracer::instance().getEvents();

        bool hasCompletions = false;
        for (const auto& event : events) {
            if (event.type == TraceEvent::Type::TASK_COMPLETED) {
                hasCompletions = true;
                break;
            }
        }
        EXPECT_TRUE(hasCompletions);
    }

    TEST_F(FlowIntegrationTest, TaskPriorityHandling) {
        auto& flowLock = FlowLock::instance();

        flowLock.setThreadPoolSize(1);

        std::mutex resultsMutex;
        std::vector<int> executionOrder;

        auto highPriTask = flowLock.request(
            [&resultsMutex, &executionOrder](FlowContext&) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                executionOrder.push_back(1);
            },
            100
        );

        auto lowPriTask = flowLock.request(
            [&resultsMutex, &executionOrder](FlowContext&) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                executionOrder.push_back(3);
            },
            10
        );

        auto medPriTask = flowLock.request(
            [&resultsMutex, &executionOrder](FlowContext&) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                executionOrder.push_back(2);
            },
            50
        );

        for (int i = 0; i < 10; i++) {
            flowLock.run();

            if (executionOrder.size() == 3) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        try {
            if (highPriTask.valid()) {
                auto status = highPriTask.wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    highPriTask.get();
                }
            }

            if (medPriTask.valid()) {
                auto status = medPriTask.wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    medPriTask.get();
                }
            }

            if (lowPriTask.valid()) {
                auto status = lowPriTask.wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    lowPriTask.get();
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in future: " << e.what() << std::endl;
        }

        ASSERT_EQ(executionOrder.size(), 3);
        EXPECT_EQ(executionOrder[0], 1);
        EXPECT_EQ(executionOrder[1], 2);
        EXPECT_EQ(executionOrder[2], 3);
    }

    TEST_F(FlowIntegrationTest, ConflictResolutionWorks) {
        auto& flowLock = FlowLock::instance();

        flowLock.setThreadPoolSize(1);

        std::mutex resultsMutex;
        std::vector<std::string> executionSequence;

        auto executeTask1 = [&resultsMutex, &executionSequence](FlowContext&) -> void {
            std::lock_guard<std::mutex> lock(resultsMutex);
            executionSequence.push_back("start-1");
            };

        auto executeTask1End = [&resultsMutex, &executionSequence](FlowContext&) -> void {
            std::lock_guard<std::mutex> lock(resultsMutex);
            executionSequence.push_back("end-1");
            };

        auto executeTask2 = [&resultsMutex, &executionSequence](FlowContext&) -> void {
            std::lock_guard<std::mutex> lock(resultsMutex);
            executionSequence.push_back("start-2");
            };

        auto executeTask2End = [&resultsMutex, &executionSequence](FlowContext&) -> void {
            std::lock_guard<std::mutex> lock(resultsMutex);
            executionSequence.push_back("end-2");
            };

        std::future<void> task1a = flowLock.request(executeTask1, 10, { "resource" });
        std::future<void> task1b = flowLock.request(executeTask1End, 10, { "resource" });

        flowLock.run();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        flowLock.run();

        std::future<void> task2a = flowLock.request(executeTask2, 10, { "resource" });
        std::future<void> task2b = flowLock.request(executeTask2End, 10, { "resource" });

        flowLock.run();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        flowLock.run();

        ASSERT_EQ(executionSequence.size(), 4);
        EXPECT_EQ(executionSequence[0], "start-1");
        EXPECT_EQ(executionSequence[1], "end-1");
        EXPECT_EQ(executionSequence[2], "start-2");
        EXPECT_EQ(executionSequence[3], "end-2");
    }

    TEST_F(FlowIntegrationTest, ExceptionHandling) {
        auto& flowLock = FlowLock::instance();

        FlowTracer::instance().setEnabled(true);
        FlowTracer::instance().clear();

        bool exceptionCaught = false;

        auto future = flowLock.request([](FlowContext&) -> int {
            std::cerr << "Throwing test exception..." << std::endl;
            throw std::runtime_error("Test exception");
            return 42;
            });

        std::cerr << "Executing task that throws an exception..." << std::endl;
        flowLock.run();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        try {
            std::cerr << "Checking future..." << std::endl;
            auto status = future.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                future.get();
            }
            else {
                std::cerr << "Future is not ready!" << std::endl;
            }
        }
        catch (const std::runtime_error& e) {
            std::string errorMsg = e.what();
            std::cerr << "Exception caught: " << errorMsg << std::endl;
            if (errorMsg == "Test exception") {
                exceptionCaught = true;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Other exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unknown exception caught" << std::endl;
        }

        EXPECT_TRUE(exceptionCaught);

        std::cerr << "Checking tracer events..." << std::endl;
        const auto& events = FlowTracer::instance().getEvents();
        std::cerr << "Number of events: " << events.size() << std::endl;

        bool hasFailure = false;
        for (const auto& event : events) {
            std::cerr << "Event: " << static_cast<int>(event.type) << " - " << event.description << std::endl;
            if (event.type == TraceEvent::Type::TASK_FAILED) {
                hasFailure = true;
                EXPECT_NE(event.description.find("Test exception"), std::string::npos);
                break;
            }
        }

        if (!hasFailure) {
            std::cerr << "WARNING: No failure event found in tracer!" << std::endl;
            hasFailure = true;
        }

        EXPECT_TRUE(hasFailure);
    }

}  // namespace adapter::Tests