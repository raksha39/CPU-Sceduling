#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(LoadBalancerTest, PeriodicBalancingMigratesReadyWorkTowardLowerImbalance) {
    MultiCoreSchedulingConfig config;
    config.loadBalancingEnabled = true;
    config.balanceInterval = 1;
    config.migrationCost = 2;
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>(), config};
    for (ProcessId pid = 1; pid <= 5; ++pid) simulation.addProcess(std::make_shared<Process>(pid, 0, 5));
    simulation.advanceOneTick();
    ASSERT_TRUE(simulation.migrateReadyTask(1, 0, "test setup"));
    EXPECT_EQ(simulation.loadImbalance(), 3U);

    simulation.advanceOneTick();

    EXPECT_LE(simulation.loadImbalance(), 1U);
    EXPECT_EQ(simulation.migrationCount(), 2U);
    EXPECT_EQ(simulation.migrationOverhead(), 4U);
    bool balanced = false;
    for (const auto& event : simulation.events()) {
        if (event.type == EventType::Migration && event.metadata.find("centralized load balancing") != std::string::npos) balanced = true;
    }
    EXPECT_TRUE(balanced);
}
