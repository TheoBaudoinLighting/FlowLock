#include "FlowLock/Core/FlowSection.h"
#include "FlowLock/Utils/FlowTracer.h"

namespace adapter {

    FlowSection::FlowSection(const std::string& name, uint32_t priority,
        const std::vector<std::string>& tags)
        : sectionName(name), priority(priority), allTags(tags) {

        allTags.push_back("section:" + name);

        FlowTracer::instance().addEvent(
            TraceEvent::Type::TASK_QUEUED,
            "Section started: " + name,
            std::nullopt,
            std::nullopt,
            allTags
        );
    }

    FlowSection::~FlowSection() {
        FlowTracer::instance().addEvent(
            TraceEvent::Type::TASK_COMPLETED,
            "Section ended: " + sectionName,
            std::nullopt,
            std::nullopt,
            allTags
        );
    }

} // namespace adapter