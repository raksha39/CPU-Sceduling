#pragma once

#include "cpu.h"
#include "event.h"
#include "metrics.h"
#include "scheduler.h"

#include <memory>
#include <vector>

namespace scheduler {

struct MultiCoreSchedulingConfig {
    Tick migrationCost{0};
    bool loadBalancingEnabled{false};
    Tick balanceInterval{0};
    bool workStealingEnabled{false};
};

class SimulationEngine {
public:
    explicit SimulationEngine(CpuId coreCount);
    SimulationEngine(CpuId coreCount, std::unique_ptr<Scheduler> scheduler);
    SimulationEngine(CpuId coreCount, std::unique_ptr<Scheduler> scheduler,
                     MultiCoreSchedulingConfig config);

    [[nodiscard]] Tick now() const noexcept;
    [[nodiscard]] const std::vector<Cpu>& cpus() const noexcept;
    [[nodiscard]] const std::vector<SchedulerEvent>& events() const noexcept;
    [[nodiscard]] const Scheduler& schedulerForCpu(CpuId cpuId) const;
    [[nodiscard]] MetricsSnapshot metrics() const;
    [[nodiscard]] const MultiCoreSchedulingConfig& schedulingConfig() const noexcept;
    [[nodiscard]] std::uint64_t migrationCount() const noexcept;
    [[nodiscard]] Tick migrationOverhead() const noexcept;
    [[nodiscard]] std::size_t load(CpuId cpuId) const;
    [[nodiscard]] std::size_t loadImbalance() const;
    [[nodiscard]] bool hasWork() const noexcept;

    void addProcess(std::shared_ptr<Process> process);
    // Moves one READY affinity-eligible task; returns false when no such task exists.
    bool migrateReadyTask(CpuId sourceCpu, CpuId destinationCpu, std::string reason = "manual migration");
    void advanceOneTick();

private:
    void admitArrivals();
    void dispatchIdleCpus();
    void preemptRunningCpus();
    void performLoadBalancing();
    void performWorkStealing();
    void preemptCpu(Cpu& cpu, std::string metadata);
    void recordQueueTransitions();
    [[nodiscard]] CpuId placeProcess(const Process& process) const;
    void recordEvent(EventType type, std::optional<ProcessId> processId,
                     std::optional<CpuId> cpuId, std::string metadata = {});

    Tick now_{0};
    std::vector<Cpu> cpus_;
    std::vector<std::shared_ptr<Process>> processes_;
    std::vector<std::unique_ptr<Scheduler>> schedulers_;
    std::vector<SchedulerEvent> events_;
    std::vector<std::optional<ProcessId>> lastProcessByCpu_;
    MultiCoreSchedulingConfig schedulingConfig_;
    std::uint64_t migrationCount_{0};
    Tick migrationOverhead_{0};
};

}  // namespace scheduler
