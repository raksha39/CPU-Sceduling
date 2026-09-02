#include "run_queue.h"
#include <stdexcept>
namespace scheduler {
void ReadyQueue::enqueue(std::shared_ptr<Process> process) {
    if (!process) throw std::invalid_argument("Cannot enqueue null process");
    if (process->state() != ProcessState::Ready) throw std::logic_error("Only READY processes may enter a ready queue");
    processes_.push_back(std::move(process));
}
std::shared_ptr<Process> ReadyQueue::dequeueEligible(const CpuId cpuId) {
    for (auto it = processes_.begin(); it != processes_.end(); ++it) {
        if ((*it)->canRunOn(cpuId)) { auto process = std::move(*it); processes_.erase(it); return process; }
    }
    return nullptr;
}
bool ReadyQueue::empty() const noexcept { return processes_.empty(); }
std::size_t ReadyQueue::size() const noexcept { return processes_.size(); }
}  // namespace scheduler
