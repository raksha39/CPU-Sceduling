#include "process.h"

#include <algorithm>

namespace scheduler {

std::string toString(const ProcessState state) {
    switch (state) {
        case ProcessState::New: return "NEW";
        case ProcessState::Ready: return "READY";
        case ProcessState::Running: return "RUNNING";
        case ProcessState::Waiting: return "WAITING";
        case ProcessState::Terminated: return "TERMINATED";
    }
    return "UNKNOWN";
}

Process::Process(const ProcessId pid, const Tick arrivalTime, const Tick burstTime,
                 const int priority, std::vector<CpuId> affinity)
    : pid_(pid), arrivalTime_(arrivalTime), burstTime_(burstTime), remainingTime_(burstTime),
      priority_(priority), affinity_(std::move(affinity)) {
    if (burstTime == 0) {
        throw std::invalid_argument("Process burst time must be positive");
    }
    std::sort(affinity_.begin(), affinity_.end());
    affinity_.erase(std::unique(affinity_.begin(), affinity_.end()), affinity_.end());
}

ProcessId Process::pid() const noexcept { return pid_; }
Tick Process::arrivalTime() const noexcept { return arrivalTime_; }
Tick Process::burstTime() const noexcept { return burstTime_; }
Tick Process::remainingTime() const noexcept { return remainingTime_; }
int Process::priority() const noexcept { return priority_; }
ProcessState Process::state() const noexcept { return state_; }
std::optional<Tick> Process::startTime() const noexcept { return startTime_; }
std::optional<Tick> Process::completionTime() const noexcept { return completionTime_; }
std::optional<CpuId> Process::currentCpu() const noexcept { return currentCpu_; }
const std::vector<CpuId>& Process::affinity() const noexcept { return affinity_; }
bool Process::canRunOn(const CpuId cpuId) const noexcept {
    return affinity_.empty() || std::binary_search(affinity_.begin(), affinity_.end(), cpuId);
}

bool Process::isValidTransition(const ProcessState from, const ProcessState to) noexcept {
    if (from == to) return true;
    switch (from) {
        case ProcessState::New: return to == ProcessState::Ready;
        case ProcessState::Ready: return to == ProcessState::Running || to == ProcessState::Terminated;
        case ProcessState::Running: return to == ProcessState::Ready || to == ProcessState::Waiting || to == ProcessState::Terminated;
        case ProcessState::Waiting: return to == ProcessState::Ready || to == ProcessState::Terminated;
        case ProcessState::Terminated: return false;
    }
    return false;
}

void Process::transitionTo(const ProcessState nextState) {
    if (!isValidTransition(state_, nextState)) {
        throw std::logic_error("Invalid process state transition from " + toString(state_) + " to " + toString(nextState));
    }
    state_ = nextState;
}

void Process::dispatch(const CpuId cpuId, const Tick now) {
    if (state_ != ProcessState::Ready) throw std::logic_error("Only READY processes may be dispatched");
    if (!canRunOn(cpuId)) throw std::logic_error("CPU is outside process affinity");
    transitionTo(ProcessState::Running);
    currentCpu_ = cpuId;
    if (!startTime_) startTime_ = now;
}

void Process::executeOneTick() {
    if (state_ != ProcessState::Running) throw std::logic_error("Only RUNNING processes may execute");
    if (remainingTime_ == 0) throw std::logic_error("Terminated process cannot execute");
    --remainingTime_;
}

void Process::preempt() {
    if (state_ != ProcessState::Running || remainingTime_ == 0) {
        throw std::logic_error("Only non-exhausted RUNNING processes may be preempted");
    }
    transitionTo(ProcessState::Ready);
    currentCpu_.reset();
}

void Process::complete(const Tick now) {
    if (state_ != ProcessState::Running || remainingTime_ != 0) {
        throw std::logic_error("Only exhausted RUNNING processes may complete");
    }
    transitionTo(ProcessState::Terminated);
    completionTime_ = now;
    currentCpu_.reset();
}

}  // namespace scheduler
