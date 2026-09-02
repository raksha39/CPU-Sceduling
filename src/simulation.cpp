#include "simulation.h"

#include <stdexcept>

namespace scheduler {

SimulationEngine::SimulationEngine(const CpuId coreCount)
    : SimulationEngine(coreCount, std::make_unique<FcfsScheduler>()) {}

SimulationEngine::SimulationEngine(const CpuId coreCount, std::unique_ptr<Scheduler> scheduler)
    : scheduler_(std::move(scheduler)) {
    if (coreCount == 0) throw std::invalid_argument("Simulation requires at least one CPU");
    if (!scheduler_) throw std::invalid_argument("Simulation requires a scheduler");
    cpus_.reserve(coreCount);
    for (CpuId id = 0; id < coreCount; ++id) cpus_.emplace_back(id);
}

Tick SimulationEngine::now() const noexcept { return now_; }
const std::vector<Cpu>& SimulationEngine::cpus() const noexcept { return cpus_; }
const std::vector<SchedulerEvent>& SimulationEngine::events() const noexcept { return events_; }

void SimulationEngine::addProcess(std::shared_ptr<Process> process) {
    if (!process) throw std::invalid_argument("Cannot add null process");
    processes_.push_back(std::move(process));
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
            scheduler_->addProcess(process);
            recordEvent(EventType::ProcessArrival, process->pid(), std::nullopt);
        }
    }
}

void SimulationEngine::dispatchIdleCpus() {
    for (auto& cpu : cpus_) {
        if (!cpu.isIdle()) continue;
        auto process = scheduler_->selectNext(cpu.id());
        if (process) {
            cpu.dispatch(process, now_);
            recordEvent(EventType::Dispatch, process->pid(), cpu.id());
        }
    }
}

void SimulationEngine::advanceOneTick() {
    admitArrivals();
    dispatchIdleCpus();
    for (auto& cpu : cpus_) {
        if (cpu.isIdle()) continue;
        cpu.executeOneTick();
        if (cpu.currentProcess()->remainingTime() == 0) {
            cpu.currentProcess()->complete(now_ + 1);
            scheduler_->onProcessComplete(cpu.id(), cpu.currentProcess());
            recordEvent(EventType::ProcessCompletion, cpu.currentProcess()->pid(), cpu.id());
            cpu.release();
        } else {
            scheduler_->onTick(cpu.id(), cpu.currentProcess());
            if (scheduler_->shouldPreempt(cpu.id(), cpu.currentProcess())) {
                auto process = cpu.currentProcess();
                process->preempt();
                cpu.release();
                scheduler_->onProcessPreempt(cpu.id(), process);
                recordEvent(EventType::Preempt, process->pid(), cpu.id(), "time quantum expired");
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
