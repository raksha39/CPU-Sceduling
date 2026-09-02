#include "simulation.h"
#include <gtest/gtest.h>
using namespace scheduler;

TEST(FcfsTest, RunsTasksInArrivalThenInsertionOrderWithoutPreemption) {
    SimulationEngine simulation{1, std::make_unique<FcfsScheduler>()};
    auto first = std::make_shared<Process>(1, 0, 3);
    auto second = std::make_shared<Process>(2, 0, 1);
    simulation.addProcess(first); simulation.addProcess(second);
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(first->startTime(), 0); EXPECT_EQ(first->completionTime(), 3);
    EXPECT_EQ(second->startTime(), 3); EXPECT_EQ(second->completionTime(), 4);
    EXPECT_EQ(simulation.events().size(), 6U);
    EXPECT_EQ(simulation.events()[2].type, EventType::Dispatch);
    EXPECT_EQ(simulation.events()[3].type, EventType::ProcessCompletion);
}

TEST(FcfsTest, SelectsAnAffinityEligibleTaskForEachCpu) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    auto cpuOneOnly = std::make_shared<Process>(1, 0, 1, 0, std::vector<CpuId>{1});
    auto unrestricted = std::make_shared<Process>(2, 0, 1);
    simulation.addProcess(cpuOneOnly); simulation.addProcess(unrestricted);
    simulation.advanceOneTick();
    EXPECT_EQ(cpuOneOnly->startTime(), 0); EXPECT_EQ(unrestricted->startTime(), 0);
    EXPECT_EQ(cpuOneOnly->completionTime(), 1); EXPECT_EQ(unrestricted->completionTime(), 1);
}
