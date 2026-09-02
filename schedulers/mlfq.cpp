#include "scheduler.h"

#include <stdexcept>

namespace scheduler {

MlfqScheduler::MlfqScheduler(MlfqConfig config) : config_(std::move(config)) {
    if (config_.queueQuanta.empty()) throw std::invalid_argument("MLFQ requires at least one queue");
    for (const auto quantum : config_.queueQuanta) {
        if (quantum && *quantum == 0) throw std::invalid_argument("MLFQ quanta must be positive");
    }
    queues_.resize(config_.queueQuanta.size());
}

const MlfqConfig& MlfqScheduler::config() const noexcept { return config_; }

void MlfqScheduler::recordTransition(const Process& process, const std::uint32_t from,
                                     const std::uint32_t to, std::string reason) {
    if (from != to) transitions_.push_back({process.pid(), from, to, std::move(reason)});
}

void MlfqScheduler::enqueue(std::shared_ptr<Process> process, const std::uint32_t level,
                            const Tick readySince) {
    if (!process) throw std::invalid_argument("Cannot enqueue null process");
    if (process->state() != ProcessState::Ready) throw std::logic_error("MLFQ requires READY processes");
    process->setQueueLevel(level);
    queues_.at(level).push_back(std::move(process));
    readySince_[queues_.at(level).back()->pid()] = readySince;
}

void MlfqScheduler::addProcess(std::shared_ptr<Process> process) {
    enqueue(std::move(process), 0, now_);
}

std::shared_ptr<Process> MlfqScheduler::selectNext(const CpuId cpuId) {
    for (auto& queue : queues_) {
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            if ((*it)->canRunOn(cpuId)) {
                auto process = std::move(*it);
                queue.erase(it);
                readySince_.erase(process->pid());
                elapsedByCpu_[cpuId] = 0;
                return process;
            }
        }
    }
    return nullptr;
}

void MlfqScheduler::promoteAgedProcesses() {
    if (config_.agingThreshold == 0) return;
    for (std::uint32_t level = 1; level < queues_.size(); ++level) {
        auto& queue = queues_[level];
        for (auto it = queue.begin(); it != queue.end();) {
            const auto waitStart = readySince_.at((*it)->pid());
            if (now_ - waitStart < config_.agingThreshold) { ++it; continue; }
            auto process = std::move(*it);
            it = queue.erase(it);
            const auto destination = level - 1;
            readySince_[process->pid()] = now_;
            process->setQueueLevel(destination);
            queues_[destination].push_back(process);
            recordTransition(*process, level, destination, "aging threshold reached");
        }
    }
}

void MlfqScheduler::boostReadyProcesses() {
    if (config_.boostInterval == 0 || now_ == 0 || now_ % config_.boostInterval != 0) return;
    for (std::uint32_t level = 1; level < queues_.size(); ++level) {
        auto& queue = queues_[level];
        while (!queue.empty()) {
            auto process = std::move(queue.front());
            queue.pop_front();
            readySince_[process->pid()] = now_;
            process->setQueueLevel(0);
            queues_[0].push_back(process);
            recordTransition(*process, level, 0, "periodic priority boost");
        }
    }
}

void MlfqScheduler::onTimeAdvance(const Tick now) {
    now_ = now;
    boostReadyProcesses();
    promoteAgedProcesses();
}

void MlfqScheduler::onTick(const Tick now, const CpuId cpuId, const std::shared_ptr<Process>&) {
    now_ = now;
    ++elapsedByCpu_[cpuId];
}

bool MlfqScheduler::hasHigherReadyTask(const CpuId cpuId, const std::uint32_t runningLevel) const {
    for (std::uint32_t level = 0; level < runningLevel; ++level) {
        for (const auto& process : queues_[level]) if (process->canRunOn(cpuId)) return true;
    }
    return false;
}

bool MlfqScheduler::shouldPreempt(const CpuId cpuId, const std::shared_ptr<Process>& process) const {
    const auto level = process->queueLevel();
    if (hasHigherReadyTask(cpuId, level)) return true;
    const auto& quantum = config_.queueQuanta.at(level);
    const auto elapsed = elapsedByCpu_.find(cpuId);
    return quantum && elapsed != elapsedByCpu_.end() && elapsed->second >= *quantum;
}

void MlfqScheduler::onProcessComplete(const CpuId cpuId, const std::shared_ptr<Process>&) {
    elapsedByCpu_.erase(cpuId);
}

void MlfqScheduler::onProcessPreempt(const CpuId cpuId, std::shared_ptr<Process> process) {
    const auto from = process->queueLevel();
    const auto& quantum = config_.queueQuanta.at(from);
    const auto elapsed = elapsedByCpu_.find(cpuId);
    const bool exhaustedQuantum = quantum && elapsed != elapsedByCpu_.end() && elapsed->second >= *quantum;
    elapsedByCpu_.erase(cpuId);
    const auto destination = exhaustedQuantum ? std::min<std::uint32_t>(from + 1, queues_.size() - 1) : from;
    enqueue(process, destination, now_);
    if (destination != from) recordTransition(*process, from, destination, "time quantum exhausted");
}

void MlfqScheduler::onProcessWakeup(std::shared_ptr<Process> process) {
    const auto level = process->queueLevel();
    enqueue(std::move(process), level, now_);
}

std::size_t MlfqScheduler::readyCount() const noexcept {
    std::size_t count = 0;
    for (const auto& queue : queues_) count += queue.size();
    return count;
}

std::unique_ptr<Scheduler> MlfqScheduler::clone() const { return std::make_unique<MlfqScheduler>(config_); }

std::vector<Scheduler::QueueTransition> MlfqScheduler::takeQueueTransitions() {
    auto result = std::move(transitions_);
    transitions_.clear();
    return result;
}

}  // namespace scheduler
