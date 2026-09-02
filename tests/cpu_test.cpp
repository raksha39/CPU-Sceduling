#include "cpu.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(CpuTest, ExecutesAssignedProcessAndTracksBusyTime) {
    Cpu cpu{0};
    auto process = std::make_shared<Process>(1, 0, 1);
    process->transitionTo(ProcessState::Ready);

    cpu.dispatch(process, 0);
    cpu.executeOneTick();

    EXPECT_FALSE(cpu.isIdle());
    EXPECT_EQ(cpu.busyTime(), 1);
    EXPECT_EQ(process->remainingTime(), 0);
    cpu.release();
    EXPECT_TRUE(cpu.isIdle());
}
