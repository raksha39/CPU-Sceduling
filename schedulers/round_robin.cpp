#include "scheduler.h"
#include <stdexcept>
namespace scheduler {
RoundRobinScheduler::RoundRobinScheduler(const Tick quantum) : quantum_(quantum) {
    if (quantum == 0) throw std::invalid_argument("Round Robin quantum must be positive");
}
Tick RoundRobinScheduler::quantum() const noexcept { return quantum_; }
void RoundRobinScheduler::addProcess(std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
std::shared_ptr<Process> RoundRobinScheduler::selectNext(const CpuId cpuId) {
    auto process = readyQueue_.dequeueEligible(cpuId); if (process) elapsedByCpu_[cpuId] = 0; return process;
}
void RoundRobinScheduler::onTimeAdvance(Tick) {}
void RoundRobinScheduler::onTick(Tick, const CpuId cpuId, const std::shared_ptr<Process>&) { ++elapsedByCpu_[cpuId]; }
bool RoundRobinScheduler::shouldPreempt(const CpuId cpuId, const std::shared_ptr<Process>&) const {
    const auto it = elapsedByCpu_.find(cpuId); return it != elapsedByCpu_.end() && it->second >= quantum_;
}
void RoundRobinScheduler::onProcessComplete(const CpuId cpuId, const std::shared_ptr<Process>&) { elapsedByCpu_.erase(cpuId); }
void RoundRobinScheduler::onProcessPreempt(const CpuId cpuId, std::shared_ptr<Process> process) { elapsedByCpu_.erase(cpuId); readyQueue_.enqueue(std::move(process)); }
void RoundRobinScheduler::onProcessWakeup(std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
std::size_t RoundRobinScheduler::readyCount() const noexcept { return readyQueue_.size(); }
std::shared_ptr<Process> RoundRobinScheduler::takeMigratable(const CpuId destinationCpu) { return readyQueue_.dequeueEligible(destinationCpu); }
void RoundRobinScheduler::acceptMigrated(std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
std::unique_ptr<Scheduler> RoundRobinScheduler::clone() const { return std::make_unique<RoundRobinScheduler>(quantum_); }
std::vector<Scheduler::QueueTransition> RoundRobinScheduler::takeQueueTransitions() { return {}; }
}  // namespace scheduler
