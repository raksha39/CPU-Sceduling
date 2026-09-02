#include "process.h"

#include <gtest/gtest.h>

using namespace scheduler;

TEST(ProcessTest, DispatchExecuteAndCompleteRecordsLifecycle) {
    Process process{1, 3, 2, 10, {0, 2}};
    EXPECT_EQ(process.state(), ProcessState::New);
    EXPECT_TRUE(process.canRunOn(0));
    EXPECT_FALSE(process.canRunOn(1));

    process.transitionTo(ProcessState::Ready);
    process.dispatch(2, 3);
    process.executeOneTick();
    process.executeOneTick();
    process.complete(5);

    EXPECT_EQ(process.state(), ProcessState::Terminated);
    EXPECT_EQ(process.startTime(), 3);
    EXPECT_EQ(process.completionTime(), 5);
    EXPECT_EQ(process.remainingTime(), 0);
    EXPECT_FALSE(process.currentCpu().has_value());
}

TEST(ProcessTest, RejectsInvalidLifecycleTransitions) {
    Process process{1, 0, 1};
    EXPECT_THROW(process.transitionTo(ProcessState::Running), std::logic_error);
    EXPECT_THROW(Process(2, 0, 0), std::invalid_argument);
}
