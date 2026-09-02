#pragma once

#include "process.h"
#include "run_queue.h"

#include <memory>
#include <unordered_map>

namespace scheduler {

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void addProcess(std::shared_ptr<Process> process) = 0;
    [[nodiscard]] virtual std::shared_ptr<Process> selectNext(CpuId cpuId) = 0;
    virtual void onTick(CpuId cpuId, const std::shared_ptr<Process>& process) = 0;
    [[nodiscard]] virtual bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const = 0;
    virtual void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) = 0;
    virtual void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) = 0;
    virtual void onProcessWakeup(std::shared_ptr<Process> process) = 0;
    [[nodiscard]] virtual std::size_t readyCount() const noexcept = 0;
};

class FcfsScheduler final : public Scheduler {
public:
    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTick(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
private:
    ReadyQueue readyQueue_;
};

class RoundRobinScheduler final : public Scheduler {
public:
    explicit RoundRobinScheduler(Tick quantum);
    [[nodiscard]] Tick quantum() const noexcept;
    void addProcess(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::shared_ptr<Process> selectNext(CpuId cpuId) override;
    void onTick(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    [[nodiscard]] bool shouldPreempt(CpuId cpuId, const std::shared_ptr<Process>& process) const override;
    void onProcessComplete(CpuId cpuId, const std::shared_ptr<Process>& process) override;
    void onProcessPreempt(CpuId cpuId, std::shared_ptr<Process> process) override;
    void onProcessWakeup(std::shared_ptr<Process> process) override;
    [[nodiscard]] std::size_t readyCount() const noexcept override;
private:
    ReadyQueue readyQueue_;
    Tick quantum_;
    std::unordered_map<CpuId, Tick> elapsedByCpu_;
};

}  // namespace scheduler
