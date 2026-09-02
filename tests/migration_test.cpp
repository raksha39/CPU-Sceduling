#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(MigrationTest, MovesReadyTaskRespectsAffinityAndAccountsConfiguredCost) {
    MultiCoreSchedulingConfig config;
    config.migrationCost = 3;
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>(), config};
    simulation.addProcess(std::make_shared<Process>(1, 0, 4));
    simulation.addProcess(std::make_shared<Process>(2, 0, 4));
    auto movable = std::make_shared<Process>(3, 0, 4);
    simulation.addProcess(movable);
    simulation.advanceOneTick();

    ASSERT_TRUE(simulation.migrateReadyTask(0, 1));
    EXPECT_EQ(movable->migrationCount(), 1U);
    EXPECT_EQ(simulation.migrationCount(), 1U);
    EXPECT_EQ(simulation.migrationOverhead(), 3U);
    EXPECT_EQ(simulation.schedulerForCpu(0).readyCount(), 0U);
    EXPECT_EQ(simulation.schedulerForCpu(1).readyCount(), 1U);
    EXPECT_EQ(simulation.events().back().type, EventType::Migration);
}

TEST(MigrationTest, RefusesMigrationWhenDestinationViolatesAffinity) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    simulation.addProcess(std::make_shared<Process>(1, 0, 4, 0, std::vector<CpuId>{0}));
    simulation.addProcess(std::make_shared<Process>(2, 0, 4));
    auto restricted = std::make_shared<Process>(3, 0, 4, 0, std::vector<CpuId>{0});
    simulation.addProcess(restricted);
    simulation.advanceOneTick();

    EXPECT_FALSE(simulation.migrateReadyTask(0, 1));
    EXPECT_EQ(restricted->migrationCount(), 0U);
}
