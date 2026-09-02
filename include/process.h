#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace scheduler {

using ProcessId = std::uint64_t;
using Tick = std::uint64_t;
using CpuId = std::uint32_t;

enum class ProcessState { New, Ready, Running, Waiting, Terminated };

[[nodiscard]] std::string toString(ProcessState state);

class Process {
public:
    Process(ProcessId pid, Tick arrivalTime, Tick burstTime, int priority = 0,
            std::vector<CpuId> affinity = {});

    [[nodiscard]] ProcessId pid() const noexcept;
    [[nodiscard]] Tick arrivalTime() const noexcept;
    [[nodiscard]] Tick burstTime() const noexcept;
    [[nodiscard]] Tick remainingTime() const noexcept;
    [[nodiscard]] int priority() const noexcept;
    [[nodiscard]] ProcessState state() const noexcept;
    [[nodiscard]] std::optional<Tick> startTime() const noexcept;
    [[nodiscard]] std::optional<Tick> completionTime() const noexcept;
    [[nodiscard]] std::optional<Tick> waitingTime() const noexcept;
    [[nodiscard]] std::optional<Tick> turnaroundTime() const noexcept;
    [[nodiscard]] std::optional<Tick> responseTime() const noexcept;
    [[nodiscard]] std::optional<CpuId> currentCpu() const noexcept;
    [[nodiscard]] std::uint32_t queueLevel() const noexcept;
    [[nodiscard]] std::uint64_t migrationCount() const noexcept;
    [[nodiscard]] const std::vector<CpuId>& affinity() const noexcept;
    [[nodiscard]] bool canRunOn(CpuId cpuId) const noexcept;

    void transitionTo(ProcessState nextState);
    void dispatch(CpuId cpuId, Tick now);
    void executeOneTick();
    void preempt();
    void complete(Tick now);
    void setQueueLevel(std::uint32_t level) noexcept;
    void recordMigration() noexcept;

private:
    [[nodiscard]] static bool isValidTransition(ProcessState from, ProcessState to) noexcept;

    ProcessId pid_;
    Tick arrivalTime_;
    Tick burstTime_;
    Tick remainingTime_;
    int priority_;
    ProcessState state_{ProcessState::New};
    std::optional<Tick> startTime_;
    std::optional<Tick> completionTime_;
    std::optional<CpuId> currentCpu_;
    std::uint32_t queueLevel_{0};
    std::uint64_t migrationCount_{0};
    std::vector<CpuId> affinity_;
};

}  // namespace scheduler
