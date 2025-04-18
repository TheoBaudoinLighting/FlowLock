#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <chrono>

namespace Volvic::Ticking {

    class FlowContext {
    public:
        FlowContext(uint32_t threadId, uint64_t logicalTick, bool enableProfiling = false);

        uint32_t getThreadId() const;
        uint64_t getLogicalTick() const;
        bool isProfilingEnabled() const;

        void startProfiling(const std::string& label);
        void endProfiling();

        struct ProfileData {
            std::string label;
            std::chrono::steady_clock::time_point startTime;
            std::chrono::steady_clock::time_point endTime;
            std::chrono::nanoseconds duration() const;
        };

        std::optional<ProfileData> getLastProfileData() const;

    private:
        uint32_t threadId;
        uint64_t logicalTick;
        bool profilingEnabled;
        std::optional<ProfileData> currentProfile;
    };

} // namespace Volvic::Ticking