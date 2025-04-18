#include "FlowLock/Core/FlowSection.h"
#include "FlowLock/Utils/FlowTracer.h"

namespace adapter {

    FlowSection::FlowSection(const std::string& name, uint32_t priority,
        const std::vector<std::string>& tags)
        : sectionName(name), priority(priority), allTags(tags) {

        allTags.push_back("section:" + name);

        // Optionally log the section creation via the tracer
        FlowTracer::instance().addEvent(
            TraceEvent::Type::TASK_QUEUED,
            "Section started: " + name,
            std::nullopt,
            std::nullopt,
            allTags
        );
    }

    FlowSection::~FlowSection() {
        // Optionally log the section completion via the tracer
        FlowTracer::instance().addEvent(
            TraceEvent::Type::TASK_COMPLETED,
            "Section ended: " + sectionName,
            std::nullopt,
            std::nullopt,
            allTags
        );
    }

} // namespace adapter