#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace scheduler::runtime {

using WorkerId = std::uint32_t;
using TaskFunction = std::function<void()>;

struct RuntimeStats {
    std::uint64_t submitted{};
    std::uint64_t completed{};
    std::uint64_t stolen{};
};

class ThreadPool {
public:
    explicit ThreadPool(std::size_t workerCount, bool workStealingEnabled = true);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void start();
    void submit(TaskFunction task, std::vector<WorkerId> affinity = {});
    void submitToWorker(TaskFunction task, WorkerId preferredWorker,
                        std::vector<WorkerId> affinity = {});
    void shutdown();  // Stops accepting tasks, drains submitted work, and joins all workers.

    [[nodiscard]] std::size_t workerCount() const noexcept;
    [[nodiscard]] RuntimeStats stats() const noexcept;

private:
    struct Task {
        TaskFunction function;
        std::vector<WorkerId> affinity;
    };
    struct Worker {
        std::mutex mutex;
        std::deque<Task> queue;
        std::atomic<std::size_t> queued{0};
        std::thread thread;
    };

    void submitImpl(TaskFunction task, WorkerId target, std::vector<WorkerId> affinity);
    void workerLoop(WorkerId workerId);
    [[nodiscard]] bool isEligible(const Task& task, WorkerId workerId) const noexcept;
    [[nodiscard]] bool tryTakeLocal(WorkerId workerId, Task& task);
    [[nodiscard]] bool trySteal(WorkerId thiefId, Task& task);
    void execute(Task task);
    [[nodiscard]] WorkerId chooseWorker(const std::vector<WorkerId>& affinity) const;
    void validateAffinity(const std::vector<WorkerId>& affinity) const;

    std::vector<std::unique_ptr<Worker>> workers_;
    bool workStealingEnabled_;
    std::atomic<bool> accepting_{false};
    std::atomic<bool> stopRequested_{false};
    std::atomic<std::size_t> pendingTasks_{0};
    std::atomic<std::size_t> activeTasks_{0};
    std::atomic<std::uint64_t> submitted_{0};
    std::atomic<std::uint64_t> completed_{0};
    std::atomic<std::uint64_t> stolen_{0};
    std::mutex lifecycleMutex_;
    std::mutex waitMutex_;
    std::condition_variable workAvailable_;
    std::condition_variable idle_;
};

}  // namespace scheduler::runtime
