#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(MetricsTest, CalculatesPerProcessAndAggregateLifecycleMetrics) {
    SimulationEngine simulation{1, std::make_unique<PriorityScheduler>()};
    auto low = std::make_shared<Process>(1, 0, 5, 1);
    auto high = std::make_shared<Process>(2, 2, 2, 5);
    simulation.addProcess(low);
    simulation.addProcess(high);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(low->waitingTime(), 2);
    EXPECT_EQ(low->turnaroundTime(), 7);
    EXPECT_EQ(low->responseTime(), 0);
    EXPECT_EQ(high->waitingTime(), 0);
    EXPECT_EQ(high->turnaroundTime(), 2);
    EXPECT_EQ(high->responseTime(), 0);
    const auto metrics = simulation.metrics();
    EXPECT_EQ(metrics.completedProcesses, 2U);
    EXPECT_DOUBLE_EQ(metrics.averageWaitingTime, 1.0);
    EXPECT_DOUBLE_EQ(metrics.averageTurnaroundTime, 4.5);
    EXPECT_DOUBLE_EQ(metrics.averageResponseTime, 0.0);
}
