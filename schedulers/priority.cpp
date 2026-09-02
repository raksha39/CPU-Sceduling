#include "scheduler.h"

namespace scheduler {

PriorityScheduler::PriorityScheduler(const bool higherValueHigher)
    : higherValueHigher_(higherValueHigher), readyQueue_(LowerPrecedence{higherValueHigher}) {}

bool PriorityScheduler::LowerPrecedence::operator()(const Entry& left, const Entry& right) const noexcept {
    if (left.process->priority() != right.process->priority()) {
        return higherValueHigher ? left.process->priority() < right.process->priority()
                                 : left.process->priority() > right.process->priority();
    }
    return left.sequence > right.sequence;
}

void PriorityScheduler::enqueue(std::shared_ptr<Process> process) {
    const auto [it, inserted] = sequenceByPid_.try_emplace(process->pid(), nextSequence_);
    if (inserted) ++nextSequence_;
    readyQueue_.push({std::move(process), it->second});
}

void PriorityScheduler::addProcess(std::shared_ptr<Process> process) { enqueue(std::move(process)); }

std::shared_ptr<Process> PriorityScheduler::selectNext(const CpuId cpuId) {
    std::vector<Entry> deferred;
    std::shared_ptr<Process> selected;
    while (!readyQueue_.empty()) {
        auto entry = readyQueue_.top();
        readyQueue_.pop();
        if (entry.process->canRunOn(cpuId)) { selected = std::move(entry.process); break; }
        deferred.push_back(std::move(entry));
    }
    for (auto& entry : deferred) readyQueue_.push(std::move(entry));
    return selected;
}

void PriorityScheduler::onTimeAdvance(Tick) {}
void PriorityScheduler::onTick(Tick, CpuId, const std::shared_ptr<Process>&) {}
bool PriorityScheduler::outranks(const Process& candidate, const Process& running) const noexcept {
    return higherValueHigher_ ? candidate.priority() > running.priority()
                              : candidate.priority() < running.priority();
}
bool PriorityScheduler::shouldPreempt(const CpuId cpuId, const std::shared_ptr<Process>& process) const {
    auto candidates = readyQueue_;
    while (!candidates.empty()) {
        const auto& candidate = candidates.top().process;
        if (candidate->canRunOn(cpuId)) return outranks(*candidate, *process);
        candidates.pop();
    }
    return false;
}
void PriorityScheduler::onProcessComplete(CpuId, const std::shared_ptr<Process>&) {}
void PriorityScheduler::onProcessPreempt(CpuId, std::shared_ptr<Process> process) { enqueue(std::move(process)); }
void PriorityScheduler::onProcessWakeup(std::shared_ptr<Process> process) { enqueue(std::move(process)); }
std::size_t PriorityScheduler::readyCount() const noexcept { return readyQueue_.size(); }
std::shared_ptr<Process> PriorityScheduler::takeMigratable(const CpuId destinationCpu) {
    return selectNext(destinationCpu);
}
void PriorityScheduler::acceptMigrated(std::shared_ptr<Process> process) { enqueue(std::move(process)); }
std::unique_ptr<Scheduler> PriorityScheduler::clone() const { return std::make_unique<PriorityScheduler>(higherValueHigher_); }
std::vector<Scheduler::QueueTransition> PriorityScheduler::takeQueueTransitions() { return {}; }

}  // namespace scheduler
