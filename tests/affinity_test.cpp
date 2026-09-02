#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(AffinityTest, RestrictedProcessOnlyRunsOnAllowedCpu) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    auto restricted = std::make_shared<Process>(1, 0, 3, 0, std::vector<CpuId>{1});
    simulation.addProcess(restricted);
    simulation.advanceOneTick();
    ASSERT_TRUE(restricted->currentCpu().has_value());
    EXPECT_EQ(*restricted->currentCpu(), 1U);
}

TEST(AffinityTest, RejectsAffinityThatExcludesAllSimulatedCpus) {
    SimulationEngine simulation{2, std::make_unique<FcfsScheduler>()};
    EXPECT_THROW(simulation.addProcess(std::make_shared<Process>(1, 0, 1, 0, std::vector<CpuId>{2})),
                 std::invalid_argument);
}
