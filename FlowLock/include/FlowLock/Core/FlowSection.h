#pragma once

#include "FlowLock/FlowLockImpl.h"
#include <string>
#include <vector>
#include <future>

namespace adapter {

    class FlowSection {
    public:
        FlowSection(const std::string& name, uint32_t priority = 0,
            const std::vector<std::string>& tags = {});
        ~FlowSection();

        FlowSection(const FlowSection&) = delete;
        FlowSection(FlowSection&&) = delete;
        FlowSection& operator=(const FlowSection&) = delete;
        FlowSection& operator=(FlowSection&&) = delete;

        template<typename F>
        auto operator<<(F&& func) {
            return FlowLockImpl::instance().request(std::forward<F>(func), priority, allTags);
        }

    private:
        std::string sectionName;
        uint32_t priority;
        std::vector<std::string> allTags;
    };

} // namespace adapter