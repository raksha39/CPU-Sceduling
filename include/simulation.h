#pragma once

#include "cpu.h"
#include "event.h"
#include "scheduler.h"

#include <memory>
#include <vector>

namespace scheduler {

class SimulationEngine {
public:
    explicit SimulationEngine(CpuId coreCount);
    SimulationEngine(CpuId coreCount, std::unique_ptr<Scheduler> scheduler);

    [[nodiscard]] Tick now() const noexcept;
    [[nodiscard]] const std::vector<Cpu>& cpus() const noexcept;
    [[nodiscard]] const std::vector<SchedulerEvent>& events() const noexcept;
    [[nodiscard]] bool hasWork() const noexcept;

    void addProcess(std::shared_ptr<Process> process);
    void advanceOneTick();

private:
    void admitArrivals();
    void dispatchIdleCpus();
    void recordEvent(EventType type, std::optional<ProcessId> processId,
                     std::optional<CpuId> cpuId, std::string metadata = {});

    Tick now_{0};
    std::vector<Cpu> cpus_;
    std::vector<std::shared_ptr<Process>> processes_;
    std::unique_ptr<Scheduler> scheduler_;
    std::vector<SchedulerEvent> events_;
};

}  // namespace scheduler
