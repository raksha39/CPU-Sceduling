#pragma once

#include "process.h"

#include <memory>
#include <vector>

namespace scheduler {

struct MetricsSnapshot {
    std::size_t completedProcesses{};
    double averageWaitingTime{};
    double averageTurnaroundTime{};
    double averageResponseTime{};
};

class Metrics {
public:
    [[nodiscard]] static MetricsSnapshot calculate(const std::vector<std::shared_ptr<Process>>& processes);
};

}  // namespace scheduler
