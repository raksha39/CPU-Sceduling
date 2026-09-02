#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(PriorityTest, HigherPriorityArrivalPreemptsBeforeTheNextCpuTick) {
    SimulationEngine simulation{1, std::make_unique<PriorityScheduler>()};
    auto low = std::make_shared<Process>(1, 0, 5, 1);
    auto high = std::make_shared<Process>(2, 2, 2, 5);
    simulation.addProcess(low);
    simulation.addProcess(high);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(low->startTime(), 0);
    EXPECT_EQ(low->completionTime(), 7);
    EXPECT_EQ(high->startTime(), 2);
    EXPECT_EQ(high->completionTime(), 4);
    ASSERT_GE(simulation.events().size(), 1U);
    EXPECT_EQ(simulation.events()[3].type, EventType::Preempt);
    EXPECT_EQ(simulation.events()[3].timestamp, 2);

    std::size_t contextSwitches = 0;
    for (const auto& event : simulation.events()) if (event.type == EventType::ContextSwitch) ++contextSwitches;
    EXPECT_EQ(contextSwitches, 2U);
}

TEST(PriorityTest, EqualPriorityDoesNotPreemptAndUsesArrivalOrder) {
    SimulationEngine simulation{1, std::make_unique<PriorityScheduler>()};
    auto first = std::make_shared<Process>(1, 0, 3, 4);
    auto second = std::make_shared<Process>(2, 1, 1, 4);
    simulation.addProcess(first);
    simulation.addProcess(second);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(first->completionTime(), 3);
    EXPECT_EQ(second->startTime(), 3);
    for (const auto& event : simulation.events()) EXPECT_NE(event.type, EventType::Preempt);
}

TEST(PriorityTest, SupportsLowerNumericValueAsHigherPriority) {
    SimulationEngine simulation{1, std::make_unique<PriorityScheduler>(false)};
    auto lowPrecedence = std::make_shared<Process>(1, 0, 3, 10);
    auto highPrecedence = std::make_shared<Process>(2, 1, 1, 1);
    simulation.addProcess(lowPrecedence);
    simulation.addProcess(highPrecedence);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(highPrecedence->startTime(), 1);
    EXPECT_EQ(highPrecedence->completionTime(), 2);
}
