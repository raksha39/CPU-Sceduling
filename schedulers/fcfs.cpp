#include "scheduler.h"
namespace scheduler {
void FcfsScheduler::addProcess(std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
std::shared_ptr<Process> FcfsScheduler::selectNext(const CpuId cpuId) { return readyQueue_.dequeueEligible(cpuId); }
void FcfsScheduler::onTick(CpuId, const std::shared_ptr<Process>&) {}
bool FcfsScheduler::shouldPreempt(CpuId, const std::shared_ptr<Process>&) const { return false; }
void FcfsScheduler::onProcessComplete(CpuId, const std::shared_ptr<Process>&) {}
void FcfsScheduler::onProcessPreempt(CpuId, std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
void FcfsScheduler::onProcessWakeup(std::shared_ptr<Process> process) { readyQueue_.enqueue(std::move(process)); }
std::size_t FcfsScheduler::readyCount() const noexcept { return readyQueue_.size(); }
}  // namespace scheduler
