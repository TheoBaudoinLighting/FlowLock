#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <chrono>
#include <atomic>

namespace adapter {

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
    
    void setTimeout(std::chrono::milliseconds timeout);
    bool isTimedOut() const;
    
    void requestCancellation();
    bool isCancellationRequested() const;
    
    bool shouldContinue() const;

private:
    uint32_t threadId;
    uint64_t logicalTick;
    bool profilingEnabled;
    std::optional<ProfileData> currentProfile;
    
    std::optional<std::chrono::steady_clock::time_point> deadlineTime;
    std::atomic<bool> cancellationRequested{false};
};

} // namespace adapter