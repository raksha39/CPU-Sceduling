#include "event.h"
namespace scheduler {
std::string toString(const EventType type) {
    switch (type) {
        case EventType::ProcessArrival: return "PROCESS_ARRIVAL";
        case EventType::Dispatch: return "DISPATCH";
        case EventType::Preempt: return "PREEMPT";
        case EventType::ContextSwitch: return "CONTEXT_SWITCH";
        case EventType::ProcessCompletion: return "PROCESS_COMPLETION";
        case EventType::QueueChange: return "QUEUE_CHANGE";
        case EventType::Migration: return "MIGRATION";
        case EventType::CpuIdle: return "CPU_IDLE";
    }
    return "UNKNOWN";
}
}  // namespace scheduler
