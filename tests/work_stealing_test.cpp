#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(WorkStealingTest, IdleCpuStealsEligibleReadyTaskFromBusyVictim) {
    MultiCoreSchedulingConfig config;
    config.workStealingEnabled = true;
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>(), config};
    simulation.addProcess(std::make_shared<Process>(1, 0, 5));
    simulation.addProcess(std::make_shared<Process>(2, 0, 1));
    auto stolen = std::make_shared<Process>(3, 0, 5);
    simulation.addProcess(stolen);
    simulation.addProcess(std::make_shared<Process>(4, 0, 5));
    simulation.addProcess(std::make_shared<Process>(5, 0, 5));
    simulation.advanceOneTick();
    ASSERT_TRUE(simulation.migrateReadyTask(1, 0, "test setup"));

    simulation.advanceOneTick();

    ASSERT_TRUE(stolen->currentCpu().has_value());
    EXPECT_EQ(*stolen->currentCpu(), 1U);
    bool stolenEvent = false;
    for (const auto& event : simulation.events()) {
        if (event.type == EventType::Migration && event.metadata.find("work stealing") != std::string::npos) stolenEvent = true;
    }
    EXPECT_TRUE(stolenEvent);
}
