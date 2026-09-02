#include "event.h"
namespace scheduler {
std::string toString(const EventType type) {
    switch (type) {
        case EventType::ProcessArrival: return "PROCESS_ARRIVAL";
        case EventType::Dispatch: return "DISPATCH";
        case EventType::Preempt: return "PREEMPT";
        case EventType::ProcessCompletion: return "PROCESS_COMPLETION";
        case EventType::CpuIdle: return "CPU_IDLE";
    }
    return "UNKNOWN";
}
}  // namespace scheduler
