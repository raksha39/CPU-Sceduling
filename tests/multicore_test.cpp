#include "simulation.h"

#include <gtest/gtest.h>

#include <set>

using namespace scheduler;

TEST(MulticoreTest, OneCpuRetainsSingleCoreExecutionSemantics) {
    SimulationEngine simulation{1, std::make_unique<FcfsScheduler>()};
    auto process = std::make_shared<Process>(1, 0, 2);
    simulation.addProcess(process);
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(process->completionTime(), 2);
    EXPECT_EQ(simulation.cpus()[0].busyTime(), 2);
}

TEST(MulticoreTest, TwoCpusExecuteInitiallyPlacedTasksInParallel) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    auto first = std::make_shared<Process>(1, 0, 2);
    auto second = std::make_shared<Process>(2, 0, 2);
    simulation.addProcess(first);
    simulation.addProcess(second);
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(first->completionTime(), 2);
    EXPECT_EQ(second->completionTime(), 2);
    EXPECT_EQ(simulation.cpus()[0].busyTime(), 2);
    EXPECT_EQ(simulation.cpus()[1].busyTime(), 2);
}

TEST(MulticoreTest, FourCpusReceiveSimultaneousArrivalsDeterministically) {
    SimulationEngine simulation{4, std::make_unique<FcfsScheduler>()};
    for (ProcessId pid = 1; pid <= 4; ++pid) simulation.addProcess(std::make_shared<Process>(pid, 0, 1));
    simulation.advanceOneTick();

    std::set<CpuId> dispatchCpus;
    for (const auto& event : simulation.events()) {
        if (event.type == EventType::Dispatch) dispatchCpus.insert(*event.cpuId);
    }
    EXPECT_EQ(dispatchCpus, (std::set<CpuId>{0, 1, 2, 3}));
}

TEST(MulticoreTest, MoreCpusThanTasksLeaveExcessCpusIdle) {
    SimulationEngine simulation{4, std::make_unique<FcfsScheduler>()};
    simulation.addProcess(std::make_shared<Process>(1, 0, 1));
    simulation.addProcess(std::make_shared<Process>(2, 0, 1));
    simulation.advanceOneTick();

    EXPECT_EQ(simulation.cpus()[0].busyTime(), 1);
    EXPECT_EQ(simulation.cpus()[1].busyTime(), 1);
    EXPECT_EQ(simulation.cpus()[2].busyTime(), 0);
    EXPECT_EQ(simulation.cpus()[3].busyTime(), 0);
    for (const auto& cpu : simulation.cpus()) EXPECT_TRUE(cpu.isIdle());
}

TEST(MulticoreTest, EveryCpuCanBeBusyWithoutDuplicatingAProcess) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    auto first = std::make_shared<Process>(1, 0, 3);
    auto second = std::make_shared<Process>(2, 0, 3);
    simulation.addProcess(first);
    simulation.addProcess(second);
    simulation.advanceOneTick();

    ASSERT_FALSE(simulation.cpus()[0].isIdle());
    ASSERT_FALSE(simulation.cpus()[1].isIdle());
    EXPECT_NE(simulation.cpus()[0].currentProcess()->pid(), simulation.cpus()[1].currentProcess()->pid());
}

TEST(MulticoreTest, EmptySimulationLeavesAllCpusIdle) {
    SimulationEngine simulation{4, std::make_unique<FcfsScheduler>()};
    EXPECT_FALSE(simulation.hasWork());
    simulation.advanceOneTick();
    for (const auto& cpu : simulation.cpus()) EXPECT_TRUE(cpu.isIdle());
}

TEST(MulticoreTest, PerCpuContextSwitchCountTracksLocalDispatchChanges) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    for (ProcessId pid = 1; pid <= 4; ++pid) simulation.addProcess(std::make_shared<Process>(pid, 0, 1));
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(simulation.cpus()[0].contextSwitchCount(), 1U);
    EXPECT_EQ(simulation.cpus()[1].contextSwitchCount(), 1U);
}
