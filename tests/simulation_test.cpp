#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(SimulationTest, DeterministicallyAdmitsDispatchesAndCompletesProcess) {
    SimulationEngine simulation{1};
    auto process = std::make_shared<Process>(7, 2, 2);
    simulation.addProcess(process);

    simulation.advanceOneTick();
    EXPECT_EQ(process->state(), ProcessState::New);
    simulation.advanceOneTick();
    EXPECT_EQ(process->state(), ProcessState::New);
    simulation.advanceOneTick();
    EXPECT_EQ(process->state(), ProcessState::Running);
    simulation.advanceOneTick();

    EXPECT_EQ(simulation.now(), 4);
    EXPECT_EQ(process->state(), ProcessState::Terminated);
    EXPECT_EQ(process->startTime(), 2);
    EXPECT_EQ(process->completionTime(), 4);
    EXPECT_EQ(simulation.cpus().front().busyTime(), 2);
}

TEST(SimulationTest, RejectsZeroCores) {
    EXPECT_THROW(SimulationEngine(0), std::invalid_argument);
}
