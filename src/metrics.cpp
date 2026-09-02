#include "metrics.h"

namespace scheduler {

MetricsSnapshot Metrics::calculate(const std::vector<std::shared_ptr<Process>>& processes) {
    MetricsSnapshot result;
    for (const auto& process : processes) {
        if (!process || process->state() != ProcessState::Terminated) continue;
        result.averageWaitingTime += static_cast<double>(*process->waitingTime());
        result.averageTurnaroundTime += static_cast<double>(*process->turnaroundTime());
        result.averageResponseTime += static_cast<double>(*process->responseTime());
        ++result.completedProcesses;
    }
    if (result.completedProcesses != 0) {
        const double count = static_cast<double>(result.completedProcesses);
        result.averageWaitingTime /= count;
        result.averageTurnaroundTime /= count;
        result.averageResponseTime /= count;
    }
    return result;
}

}  // namespace scheduler
