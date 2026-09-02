#pragma once

#include "process.h"
#include "run_queue.h"

#include <memory>
#include <deque>
#include <optional>
#include <queue>
#include <unordered_map>

namespace scheduler {

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void addProcess(std::shared_ptr<Process> process) = 0;
    [[nodiscard]] virtual std::shared_ptr<Process> selectNext(CpuId cpuId) = 0;
    virtual void onTimeAdvance(Tick now) = 0;
    virtual void onTick(Tick now, CpuId cpuId, const std::shared_ptr<Process>& process) = 0;
    [[nodiscard]] virtual bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const = 0;
    virtual void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) = 0;
    virtual void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) = 0;
    virtual void onProcessWakeup(std::shared_ptr<Process> process) = 0;
    [[nodiscard]] virtual std::size_t readyCount() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<Process> takeMigratable(CpuId destinationCpu) = 0;
    virtual void acceptMigrated(std::shared_ptr<Process> process) = 0;
    [[nodiscard]] virtual std::unique_ptr<Scheduler> clone() const = 0;
    struct QueueTransition {
        ProcessId processId;
        std::uint32_t fromLevel;
        std::uint32_t toLevel;
        std::string reason;
    };
    [[nodiscard]] virtual std::vector<QueueTransition> takeQueueTransitions() = 0;
};

class FcfsScheduler final : public Scheduler {
public:
    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTimeAdvance(Tick now) override;
    void onTick(Tick now, CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
    [[nodiscard]] std::shared_ptr<Process> takeMigratable(CpuId destinationCpu) override;
    void acceptMigrated(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::unique_ptr<Scheduler> clone() const override;
    [[nodiscard]] std::vector<QueueTransition> takeQueueTransitions() override;
private:
    ReadyQueue readyQueue_;
};

class RoundRobinScheduler final : public Scheduler {
public:
    explicit RoundRobinScheduler(Tick quantum);
    [[nodiscard]] Tick quantum() const noexcept;
    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTimeAdvance(Tick now) override;
    void onTick(Tick now, CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
    [[nodiscard]] std::shared_ptr<Process> takeMigratable(CpuId destinationCpu) override;
    void acceptMigrated(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::unique_ptr<Scheduler> clone() const override;
    [[nodiscard]] std::vector<QueueTransition> takeQueueTransitions() override;
private:
    ReadyQueue readyQueue_;
    Tick quantum_;
    std::unordered_map<CpuId, Tick> elapsedByCpu_;
};

class PriorityScheduler final : public Scheduler {
public:
    // When true, a numerically larger value is a higher priority.
    explicit PriorityScheduler(bool higherValueHigher = true);

    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTimeAdvance(Tick now) override;
    void onTick(Tick now, CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
    [[nodiscard]] std::shared_ptr<Process> takeMigratable(CpuId destinationCpu) override;
    void acceptMigrated(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::unique_ptr<Scheduler> clone() const override;
    [[nodiscard]] std::vector<QueueTransition> takeQueueTransitions() override;

private:
    struct Entry {
        std::shared_ptr<Process> process;
        std::uint64_t sequence;
    };
    struct LowerPrecedence {
        bool higherValueHigher{};
        [[nodiscard]] bool operator()(const Entry& left, const Entry& right) const noexcept;
    };

    void enqueue(std::shared_ptr<Process> process);
    [[nodiscard]] bool outranks(const Process& candidate, const Process& running) const noexcept;

    bool higherValueHigher_;
    std::uint64_t nextSequence_{0};
    std::unordered_map<ProcessId, std::uint64_t> sequenceByPid_;
    std::priority_queue<Entry, std::vector<Entry>, LowerPrecedence> readyQueue_;
};

struct MlfqConfig {
    // nullopt denotes FCFS for that queue.
    std::vector<std::optional<Tick>> queueQuanta{{2}, {4}, std::nullopt};
    Tick agingThreshold{20};       // Zero disables aging.
    Tick boostInterval{100};       // Zero disables periodic boosts.
};

class MlfqScheduler final : public Scheduler {
public:
    explicit MlfqScheduler(MlfqConfig config = {});

    [[nodiscard]] const MlfqConfig& config() const noexcept;
    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTimeAdvance(Tick now) override;
    void onTick(Tick now, CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
    [[nodiscard]] std::shared_ptr<Process> takeMigratable(CpuId destinationCpu) override;
    void acceptMigrated(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::unique_ptr<Scheduler> clone() const override;
    [[nodiscard]] std::vector<QueueTransition> takeQueueTransitions() override;

private:
    void enqueue(std::shared_ptr<Process> process, std::uint32_t level, Tick readySince);
    void promoteAgedProcesses();
    void boostReadyProcesses();
    [[nodiscard]] bool hasHigherReadyTask(CpuId cpuId, std::uint32_t runningLevel) const;
    void recordTransition(const Process& process, std::uint32_t from, std::uint32_t to, std::string reason);

    MlfqConfig config_;
    Tick now_{0};
    std::vector<std::deque<std::shared_ptr<Process>>> queues_;
    std::unordered_map<ProcessId, Tick> readySince_;
    std::unordered_map<CpuId, Tick> elapsedByCpu_;
    std::vector<QueueTransition> transitions_;
};

}  // namespace scheduler
