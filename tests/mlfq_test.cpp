#include "simulation.h"

#include <gtest/gtest.h>

using namespace scheduler;

namespace {

MlfqConfig config(const Tick aging = 0, const Tick boost = 0) {
    return MlfqConfig{{2, 4, std::nullopt}, aging, boost};
}

std::size_t queueChanges(const SimulationEngine& simulation, const std::string& reason) {
    std::size_t count = 0;
    for (const auto& event : simulation.events()) {
        if (event.type == EventType::QueueChange && event.metadata.find(reason) != std::string::npos) ++count;
    }
    return count;
}

}  // namespace

TEST(MlfqTest, CpuBoundProcessIsDemotedAfterEachFiniteQuantum) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config())};
    auto process = std::make_shared<Process>(1, 0, 10);
    simulation.addProcess(process);

    simulation.advanceOneTick();
    simulation.advanceOneTick();
    EXPECT_EQ(process->state(), ProcessState::Ready);
    EXPECT_EQ(process->queueLevel(), 1U);
    for (int i = 0; i < 4; ++i) simulation.advanceOneTick();
    EXPECT_EQ(process->queueLevel(), 2U);
    EXPECT_EQ(queueChanges(simulation, "time quantum exhausted"), 2U);
}

TEST(MlfqTest, AgingPromotesAWaitingLowerQueueTask) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config(3))};
    auto cpuBound = std::make_shared<Process>(1, 0, 20);
    simulation.addProcess(cpuBound);
    for (ProcessId pid = 2; pid < 6; ++pid) {
        simulation.addProcess(std::make_shared<Process>(pid, pid, 1));
    }

    for (int i = 0; i < 6; ++i) simulation.advanceOneTick();

    EXPECT_GE(queueChanges(simulation, "aging threshold reached"), 1U);
    EXPECT_EQ(cpuBound->queueLevel(), 0U);
}

TEST(MlfqTest, PeriodicBoostPreventsLowerQueueStarvation) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config(0, 10))};
    auto background = std::make_shared<Process>(1, 0, 30);
    auto foreground = std::make_shared<Process>(2, 6, 12);
    simulation.addProcess(background);
    simulation.addProcess(foreground);

    for (int i = 0; i < 11; ++i) simulation.advanceOneTick();

    EXPECT_GE(queueChanges(simulation, "periodic priority boost"), 1U);
    EXPECT_EQ(background->queueLevel(), 0U);
}

TEST(MlfqTest, HigherQueueArrivalPreemptsRunningLowerQueueTask) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config())};
    auto lower = std::make_shared<Process>(1, 0, 10);
    auto higher = std::make_shared<Process>(2, 3, 1);
    simulation.addProcess(lower);
    simulation.addProcess(higher);

    for (int i = 0; i < 4; ++i) simulation.advanceOneTick();

    EXPECT_EQ(lower->queueLevel(), 1U);
    EXPECT_EQ(higher->startTime(), 3);
    bool preempted = false;
    for (const auto& event : simulation.events()) {
        if (event.type == EventType::Preempt && event.processId == lower->pid()) preempted = true;
    }
    EXPECT_TRUE(preempted);
}

TEST(MlfqTest, SimultaneousArrivalsAndEqualLevelsUseFifoOrder) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config())};
    auto first = std::make_shared<Process>(1, 0, 1);
    auto second = std::make_shared<Process>(2, 0, 1);
    simulation.addProcess(first);
    simulation.addProcess(second);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(first->startTime(), 0);
    EXPECT_EQ(second->startTime(), 1);
}

TEST(MlfqTest, LongCpuBoundTaskEventuallyRunsInFcfsQueue) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config())};
    auto process = std::make_shared<Process>(1, 0, 12);
    simulation.addProcess(process);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(process->queueLevel(), 2U);
    EXPECT_EQ(queueChanges(simulation, "time quantum exhausted"), 2U);
    EXPECT_EQ(process->completionTime(), 12);
}

TEST(MlfqTest, ShortInteractiveTaskFinishesAtHighestQueueWithoutDemotion) {
    SimulationEngine simulation{1, std::make_unique<MlfqScheduler>(config())};
    auto process = std::make_shared<Process>(1, 0, 1);
    simulation.addProcess(process);

    while (simulation.hasWork()) simulation.advanceOneTick();

    EXPECT_EQ(process->queueLevel(), 0U);
    EXPECT_EQ(queueChanges(simulation, "time quantum exhausted"), 0U);
}

TEST(MlfqTest, WakeupRetainsQueueLevelForInteractiveTasks) {
    MlfqScheduler scheduler{config()};
    scheduler.onTimeAdvance(4);
    auto process = std::make_shared<Process>(1, 0, 2);
    process->transitionTo(ProcessState::Ready);
    process->setQueueLevel(1);
    scheduler.onProcessWakeup(process);

    const auto selected = scheduler.selectNext(0);
    ASSERT_EQ(selected, process);
    EXPECT_EQ(selected->queueLevel(), 1U);
}

TEST(MlfqTest, RejectsInvalidQueueConfigurations) {
    EXPECT_THROW(MlfqScheduler(MlfqConfig{{}, 1, 1}), std::invalid_argument);
    EXPECT_THROW(MlfqScheduler(MlfqConfig{{0}, 1, 1}), std::invalid_argument);
}
