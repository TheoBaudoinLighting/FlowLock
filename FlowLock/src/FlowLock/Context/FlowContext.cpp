#include "FlowLock/Context/FlowContext.h"

namespace adapter {

    FlowContext::FlowContext(uint32_t threadId, uint64_t logicalTick, bool enableProfiling)
        : threadId(threadId), logicalTick(logicalTick), profilingEnabled(enableProfiling) {
    }

    uint32_t FlowContext::getThreadId() const {
        return threadId;
    }

    uint64_t FlowContext::getLogicalTick() const {
        return logicalTick;
    }

    bool FlowContext::isProfilingEnabled() const {
        return profilingEnabled;
    }

    void FlowContext::startProfiling(const std::string& label) {
        if (!profilingEnabled) return;

        ProfileData data;
        data.label = label;
        data.startTime = std::chrono::steady_clock::now();
        currentProfile = data;
    }

    void FlowContext::endProfiling() {
        if (!profilingEnabled || !currentProfile.has_value()) return;

        currentProfile->endTime = std::chrono::steady_clock::now();
    }

    std::optional<FlowContext::ProfileData> FlowContext::getLastProfileData() const {
        return currentProfile;
    }

    std::chrono::nanoseconds FlowContext::ProfileData::duration() const {
        return endTime - startTime;
    }

} // namespace adapter