#pragma once

#include "process.h"

#include <memory>

namespace scheduler {

class Cpu {
public:
    explicit Cpu(CpuId id);

    [[nodiscard]] CpuId id() const noexcept;
    [[nodiscard]] bool isIdle() const noexcept;
    [[nodiscard]] const std::shared_ptr<Process>& currentProcess() const noexcept;
    [[nodiscard]] Tick busyTime() const noexcept;
    [[nodiscard]] std::uint64_t contextSwitchCount() const noexcept;

    void dispatch(const std::shared_ptr<Process>& process, Tick now);
    std::shared_ptr<Process> release();
    void executeOneTick();
    void recordContextSwitch() noexcept;

private:
    CpuId id_;
    std::shared_ptr<Process> currentProcess_;
    Tick busyTime_{0};
    std::uint64_t contextSwitchCount_{0};
};

}  // namespace scheduler
