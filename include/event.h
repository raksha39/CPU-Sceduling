#pragma once

#include "process.h"

#include <optional>
#include <string>
#include <vector>

namespace scheduler {
enum class EventType { ProcessArrival, Dispatch, Preempt, ContextSwitch, ProcessCompletion, QueueChange, Migration, CpuIdle };
[[nodiscard]] std::string toString(EventType type);
struct SchedulerEvent {
    Tick timestamp;
    EventType type;
    std::optional<ProcessId> processId;
    std::optional<CpuId> cpuId;
    std::string metadata;
};
}  // namespace scheduler
