#include "simulation.h"
#include <gtest/gtest.h>
using namespace scheduler;

TEST(RoundRobinTest, RequeuesAfterQuantumAndAlternatesTasks) {
    SimulationEngine simulation{1, std::make_unique<RoundRobinScheduler>(2)};
    auto first = std::make_shared<Process>(1, 0, 3);
    auto second = std::make_shared<Process>(2, 0, 3);
    simulation.addProcess(first); simulation.addProcess(second);
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(first->completionTime(), 5); EXPECT_EQ(second->completionTime(), 6);
    std::size_t preemptions = 0;
    for (const auto& event : simulation.events()) if (event.type == EventType::Preempt) ++preemptions;
    EXPECT_EQ(preemptions, 2U);
}

TEST(RoundRobinTest, DoesNotPreemptAtCompletionBoundary) {
    SimulationEngine simulation{1, std::make_unique<RoundRobinScheduler>(2)};
    auto process = std::make_shared<Process>(1, 0, 2); simulation.addProcess(process);
    while (simulation.hasWork()) simulation.advanceOneTick();
    EXPECT_EQ(process->completionTime(), 2);
    EXPECT_EQ(simulation.events().back().type, EventType::ProcessCompletion);
    for (const auto& event : simulation.events()) EXPECT_NE(event.type, EventType::Preempt);
}

TEST(RoundRobinTest, RejectsZeroQuantum) { EXPECT_THROW(RoundRobinScheduler(0), std::invalid_argument); }
