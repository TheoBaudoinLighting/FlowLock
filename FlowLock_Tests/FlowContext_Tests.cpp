#include "pch.h"

namespace adapter::Tests {

    class FlowContextTest : public ::testing::Test {
    protected:
        void SetUp() override {
        }

        void TearDown() override {
        }
    };

    TEST_F(FlowContextTest, ConstructWithThreadIdAndLogicalTick) {
        FlowContext context(42, 123);

        EXPECT_EQ(context.getThreadId(), 42);
        EXPECT_EQ(context.getLogicalTick(), 123);
        EXPECT_FALSE(context.isProfilingEnabled());
    }

    TEST_F(FlowContextTest, ProfilingDisabledByDefault) {
        FlowContext context(1, 1);

        EXPECT_FALSE(context.isProfilingEnabled());

        context.startProfiling("test");
        context.endProfiling();

        EXPECT_FALSE(context.getLastProfileData().has_value());
    }

    TEST_F(FlowContextTest, EnableProfilingWorksCorrectly) {
        FlowContext context(1, 1, true);

        EXPECT_TRUE(context.isProfilingEnabled());

        context.startProfiling("test_profile");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        context.endProfiling();

        auto profileData = context.getLastProfileData();
        EXPECT_TRUE(profileData.has_value());
        EXPECT_EQ(profileData->label, "test_profile");
        EXPECT_GT(profileData->duration().count(), 0);
    }

    TEST_F(FlowContextTest, ProfileDataDurationCalculation) {
        FlowContext context(1, 1, true);

        context.startProfiling("duration_test");
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto end = std::chrono::steady_clock::now();
        context.endProfiling();

        auto profileData = context.getLastProfileData();
        EXPECT_TRUE(profileData.has_value());

        auto expectedDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto actualDuration = profileData->duration();

        auto marginOfError = std::chrono::milliseconds(20);

        EXPECT_LE(actualDuration, expectedDuration + marginOfError);
        EXPECT_GE(actualDuration, expectedDuration - marginOfError);
    }

}  // namespace adapter::Tests