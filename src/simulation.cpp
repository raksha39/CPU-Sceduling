#include "simulation.h"

#include <stdexcept>

namespace scheduler {

SimulationEngine::SimulationEngine(const CpuId coreCount)
    : SimulationEngine(coreCount, std::make_unique<FcfsScheduler>()) {}

SimulationEngine::SimulationEngine(const CpuId coreCount, std::unique_ptr<Scheduler> scheduler)
    : SimulationEngine(coreCount, std::move(scheduler), {}) {}

SimulationEngine::SimulationEngine(const CpuId coreCount, std::unique_ptr<Scheduler> scheduler,
                                   MultiCoreSchedulingConfig config)
    : schedulingConfig_(config) {
    if (coreCount == 0) throw std::invalid_argument("Simulation requires at least one CPU");
    if (!scheduler) throw std::invalid_argument("Simulation requires a scheduler");
    if (schedulingConfig_.loadBalancingEnabled && schedulingConfig_.balanceInterval == 0) {
        throw std::invalid_argument("Load balancing requires a positive balance interval");
    }
    cpus_.reserve(coreCount);
    schedulers_.reserve(coreCount);
    for (CpuId id = 0; id < coreCount; ++id) {
        cpus_.emplace_back(id);
        schedulers_.push_back(id == 0 ? std::move(scheduler) : schedulers_.front()->clone());
    }
    lastProcessByCpu_.resize(coreCount);
}

Tick SimulationEngine::now() const noexcept { return now_; }
const std::vector<Cpu>& SimulationEngine::cpus() const noexcept { return cpus_; }
const std::vector<SchedulerEvent>& SimulationEngine::events() const noexcept { return events_; }
const Scheduler& SimulationEngine::schedulerForCpu(const CpuId cpuId) const { return *schedulers_.at(cpuId); }
MetricsSnapshot SimulationEngine::metrics() const { return Metrics::calculate(processes_); }
const MultiCoreSchedulingConfig& SimulationEngine::schedulingConfig() const noexcept { return schedulingConfig_; }
std::uint64_t SimulationEngine::migrationCount() const noexcept { return migrationCount_; }
Tick SimulationEngine::migrationOverhead() const noexcept { return migrationOverhead_; }

std::size_t SimulationEngine::load(const CpuId cpuId) const {
    const auto& cpu = cpus_.at(cpuId);
    return schedulers_.at(cpuId)->readyCount() + (cpu.isIdle() ? 0U : 1U);
}

std::size_t SimulationEngine::loadImbalance() const {
    if (cpus_.empty()) return 0;
    auto minimum = load(0);
    auto maximum = minimum;
    for (CpuId cpuId = 1; cpuId < cpus_.size(); ++cpuId) {
        minimum = std::min(minimum, load(cpuId));
        maximum = std::max(maximum, load(cpuId));
    }
    return maximum - minimum;
}

void SimulationEngine::addProcess(std::shared_ptr<Process> process) {
    if (!process) throw std::invalid_argument("Cannot add null process");
    static_cast<void>(placeProcess(*process));
    processes_.push_back(std::move(process));
}

CpuId SimulationEngine::placeProcess(const Process& process) const {
    std::optional<CpuId> selected;
    std::size_t selectedLoad = 0;
    for (const auto& cpu : cpus_) {
        if (!process.canRunOn(cpu.id())) continue;
        const auto load = schedulers_.at(cpu.id())->readyCount() + (cpu.isIdle() ? 0U : 1U);
        if (!selected || load < selectedLoad) {
            selected = cpu.id();
            selectedLoad = load;
        }
    }
    if (!selected) throw std::invalid_argument("Process affinity excludes every simulated CPU");
    return *selected;
}

bool SimulationEngine::migrateReadyTask(const CpuId sourceCpu, const CpuId destinationCpu, std::string reason) {
    if (sourceCpu >= cpus_.size() || destinationCpu >= cpus_.size()) {
        throw std::out_of_range("Migration CPU identifier is out of range");
    }
    if (sourceCpu == destinationCpu) return false;
    auto process = schedulers_.at(sourceCpu)->takeMigratable(destinationCpu);
    if (!process) return false;
    if (!process->canRunOn(destinationCpu)) throw std::logic_error("Migration violates process affinity");
    schedulers_.at(destinationCpu)->acceptMigrated(process);
    process->recordMigration();
    ++migrationCount_;
    migrationOverhead_ += schedulingConfig_.migrationCost;
    recordEvent(EventType::Migration, process->pid(), sourceCpu,
                "to CPU" + std::to_string(destinationCpu) + ": " + std::move(reason));
    return true;
}

bool SimulationEngine::hasWork() const noexcept {
    for (const auto& process : processes_) {
        if (process->state() != ProcessState::Terminated) return true;
    }
    return false;
}

void SimulationEngine::admitArrivals() {
    for (const auto& process : processes_) {
        if (process->state() == ProcessState::New && process->arrivalTime() <= now_) {
            process->transitionTo(ProcessState::Ready);
            const auto cpuId = placeProcess(*process);
            schedulers_.at(cpuId)->addProcess(process);
            recordEvent(EventType::ProcessArrival, process->pid(), std::nullopt);
        }
    }
}

void SimulationEngine::recordQueueTransitions() {
    for (CpuId cpuId = 0; cpuId < schedulers_.size(); ++cpuId) {
        for (auto& transition : schedulers_[cpuId]->takeQueueTransitions()) {
            recordEvent(EventType::QueueChange, transition.processId, cpuId,
                        "Q" + std::to_string(transition.fromLevel) + " -> Q" +
                        std::to_string(transition.toLevel) + ": " + transition.reason);
        }
    }
}

void SimulationEngine::dispatchIdleCpus() {
    for (auto& cpu : cpus_) {
        if (!cpu.isIdle()) continue;
        auto process = schedulers_.at(cpu.id())->selectNext(cpu.id());
        if (process) {
            auto& lastProcess = lastProcessByCpu_.at(cpu.id());
            if (lastProcess && *lastProcess != process->pid()) {
                cpu.recordContextSwitch();
                recordEvent(EventType::ContextSwitch, process->pid(), cpu.id(),
                            "from P" + std::to_string(*lastProcess) + " to P" + std::to_string(process->pid()));
            }
            cpu.dispatch(process, now_);
            recordEvent(EventType::Dispatch, process->pid(), cpu.id());
            lastProcess = process->pid();
        }
    }
}

void SimulationEngine::preemptCpu(Cpu& cpu, std::string metadata) {
    auto process = cpu.currentProcess();
    process->preempt();
    cpu.release();
    schedulers_.at(cpu.id())->onProcessPreempt(cpu.id(), process);
    recordEvent(EventType::Preempt, process->pid(), cpu.id(), std::move(metadata));
    recordQueueTransitions();
}

void SimulationEngine::preemptRunningCpus() {
    for (auto& cpu : cpus_) {
        if (!cpu.isIdle() && schedulers_.at(cpu.id())->shouldPreempt(cpu.id(), cpu.currentProcess())) {
            preemptCpu(cpu, "higher-priority process ready");
        }
    }
}

void SimulationEngine::performLoadBalancing() {
    if (!schedulingConfig_.loadBalancingEnabled || now_ == 0 ||
        now_ % schedulingConfig_.balanceInterval != 0) return;

    while (true) {
        CpuId source = 0;
        CpuId destination = 0;
        for (CpuId cpuId = 1; cpuId < cpus_.size(); ++cpuId) {
            if (load(cpuId) > load(source)) source = cpuId;
            if (load(cpuId) < load(destination)) destination = cpuId;
        }
        if (load(source) <= load(destination) + 1) return;
        if (!migrateReadyTask(source, destination, "centralized load balancing")) return;
    }
}

void SimulationEngine::performWorkStealing() {
    if (!schedulingConfig_.workStealingEnabled) return;
    for (const auto& idleCpu : cpus_) {
        if (!idleCpu.isIdle() || schedulers_.at(idleCpu.id())->readyCount() != 0) continue;
        std::optional<CpuId> victim;
        for (const auto& candidate : cpus_) {
            if (candidate.id() == idleCpu.id() || schedulers_.at(candidate.id())->readyCount() == 0) continue;
            if (!victim || schedulers_.at(candidate.id())->readyCount() > schedulers_.at(*victim)->readyCount()) {
                victim = candidate.id();
            }
        }
        if (victim) migrateReadyTask(*victim, idleCpu.id(), "work stealing");
    }
}

void SimulationEngine::advanceOneTick() {
    for (auto& scheduler : schedulers_) scheduler->onTimeAdvance(now_);
    recordQueueTransitions();
    admitArrivals();
    preemptRunningCpus();
    performLoadBalancing();
    dispatchIdleCpus();
    performWorkStealing();
    dispatchIdleCpus();
    for (auto& cpu : cpus_) {
        if (cpu.isIdle()) continue;
        cpu.executeOneTick();
        if (cpu.currentProcess()->remainingTime() == 0) {
            cpu.currentProcess()->complete(now_ + 1);
            schedulers_.at(cpu.id())->onProcessComplete(cpu.id(), cpu.currentProcess());
            recordEvent(EventType::ProcessCompletion, cpu.currentProcess()->pid(), cpu.id());
            cpu.release();
        } else {
            schedulers_.at(cpu.id())->onTick(now_ + 1, cpu.id(), cpu.currentProcess());
            if (schedulers_.at(cpu.id())->shouldPreempt(cpu.id(), cpu.currentProcess())) {
                preemptCpu(cpu, "scheduler preemption");
            }
        }
    }
    ++now_;
}

void SimulationEngine::recordEvent(const EventType type, std::optional<ProcessId> processId,
                                   std::optional<CpuId> cpuId, std::string metadata) {
    events_.push_back({now_, type, processId, cpuId, std::move(metadata)});
}

}  // namespace scheduler
