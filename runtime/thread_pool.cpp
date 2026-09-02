#include "runtime/thread_pool.h"

#include <algorithm>
#include <stdexcept>

namespace scheduler::runtime {

ThreadPool::ThreadPool(const std::size_t workerCount, const bool workStealingEnabled)
    : workStealingEnabled_(workStealingEnabled) {
    if (workerCount == 0) throw std::invalid_argument("ThreadPool requires at least one worker");
    workers_.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index) workers_.push_back(std::make_unique<Worker>());
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::start() {
    std::lock_guard lock(lifecycleMutex_);
    if (accepting_.load()) return;
    if (stopRequested_.load()) throw std::logic_error("ThreadPool cannot restart after shutdown");
    accepting_.store(true);
    for (WorkerId workerId = 0; workerId < workers_.size(); ++workerId) {
        workers_[workerId]->thread = std::thread(&ThreadPool::workerLoop, this, workerId);
    }
}

void ThreadPool::validateAffinity(const std::vector<WorkerId>& affinity) const {
    for (const auto workerId : affinity) {
        if (workerId >= workers_.size()) throw std::invalid_argument("Task affinity references an unknown worker");
    }
}

bool ThreadPool::isEligible(const Task& task, const WorkerId workerId) const noexcept {
    return task.affinity.empty() || std::find(task.affinity.begin(), task.affinity.end(), workerId) != task.affinity.end();
}

WorkerId ThreadPool::chooseWorker(const std::vector<WorkerId>& affinity) const {
    WorkerId selected = 0;
    bool found = false;
    std::size_t queued = 0;
    for (WorkerId workerId = 0; workerId < workers_.size(); ++workerId) {
        if (!affinity.empty() && std::find(affinity.begin(), affinity.end(), workerId) == affinity.end()) continue;
        const auto candidateQueued = workers_[workerId]->queued.load();
        if (!found || candidateQueued < queued) {
            selected = workerId;
            queued = candidateQueued;
            found = true;
        }
    }
    return selected;
}

void ThreadPool::submit(TaskFunction task, std::vector<WorkerId> affinity) {
    validateAffinity(affinity);
    std::lock_guard lock(lifecycleMutex_);
    if (!accepting_.load()) throw std::logic_error("ThreadPool is not running");
    submitImpl(std::move(task), chooseWorker(affinity), std::move(affinity));
}

void ThreadPool::submitToWorker(TaskFunction task, const WorkerId preferredWorker,
                                std::vector<WorkerId> affinity) {
    validateAffinity(affinity);
    if (preferredWorker >= workers_.size()) throw std::invalid_argument("Unknown preferred worker");
    if (!affinity.empty() && std::find(affinity.begin(), affinity.end(), preferredWorker) == affinity.end()) {
        throw std::invalid_argument("Preferred worker violates task affinity");
    }
    std::lock_guard lock(lifecycleMutex_);
    if (!accepting_.load()) throw std::logic_error("ThreadPool is not running");
    submitImpl(std::move(task), preferredWorker, std::move(affinity));
}

void ThreadPool::submitImpl(TaskFunction task, const WorkerId target, std::vector<WorkerId> affinity) {
    if (!task) throw std::invalid_argument("Cannot submit an empty task");
    auto& worker = *workers_[target];
    {
        std::lock_guard lock(worker.mutex);
        worker.queue.push_back({std::move(task), std::move(affinity)});
        worker.queued.fetch_add(1);
    }
    pendingTasks_.fetch_add(1);
    submitted_.fetch_add(1);
    workAvailable_.notify_one();
}

bool ThreadPool::tryTakeLocal(const WorkerId workerId, Task& task) {
    auto& worker = *workers_[workerId];
    std::unique_lock lock(worker.mutex);
    if (worker.queue.empty()) return false;
    task = std::move(worker.queue.front());
    worker.queue.pop_front();
    worker.queued.fetch_sub(1);
    pendingTasks_.fetch_sub(1);
    return true;
}

bool ThreadPool::trySteal(const WorkerId thiefId, Task& task) {
    if (!workStealingEnabled_) return false;
    for (WorkerId victimId = 0; victimId < workers_.size(); ++victimId) {
        if (victimId == thiefId) continue;
        auto& victim = *workers_[victimId];
        std::unique_lock lock(victim.mutex);
        for (auto it = victim.queue.end(); it != victim.queue.begin();) {
            --it;
            if (!isEligible(*it, thiefId)) continue;
            task = std::move(*it);
            victim.queue.erase(it);
            victim.queued.fetch_sub(1);
            pendingTasks_.fetch_sub(1);
            stolen_.fetch_add(1);
            return true;
        }
    }
    return false;
}

void ThreadPool::execute(Task task) {
    activeTasks_.fetch_add(1);
    try {
        task.function();
    } catch (...) {
        // A task failure must not terminate a worker or prevent shutdown from draining.
    }
    activeTasks_.fetch_sub(1);
    completed_.fetch_add(1);
    idle_.notify_all();
}

void ThreadPool::workerLoop(const WorkerId workerId) {
    while (true) {
        Task task;
        if (tryTakeLocal(workerId, task) || trySteal(workerId, task)) {
            execute(std::move(task));
            continue;
        }
        std::unique_lock lock(waitMutex_);
        workAvailable_.wait(lock, [this] { return stopRequested_.load() || pendingTasks_.load() != 0; });
        if (stopRequested_.load() && pendingTasks_.load() == 0) return;
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard lock(lifecycleMutex_);
        if (!accepting_.load() && stopRequested_.load()) return;
        accepting_.store(false);
    }
    {
        std::unique_lock lock(waitMutex_);
        idle_.wait(lock, [this] { return pendingTasks_.load() == 0 && activeTasks_.load() == 0; });
        stopRequested_.store(true);
    }
    workAvailable_.notify_all();
    for (auto& worker : workers_) {
        if (worker->thread.joinable()) worker->thread.join();
    }
}

std::size_t ThreadPool::workerCount() const noexcept { return workers_.size(); }
RuntimeStats ThreadPool::stats() const noexcept { return {submitted_.load(), completed_.load(), stolen_.load()}; }

}  // namespace scheduler::runtime
