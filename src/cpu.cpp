#include "cpu.h"

#include <stdexcept>

namespace scheduler {

Cpu::Cpu(const CpuId id) : id_(id) {}
CpuId Cpu::id() const noexcept { return id_; }
bool Cpu::isIdle() const noexcept { return !currentProcess_; }
const std::shared_ptr<Process>& Cpu::currentProcess() const noexcept { return currentProcess_; }
Tick Cpu::busyTime() const noexcept { return busyTime_; }

void Cpu::dispatch(const std::shared_ptr<Process>& process, const Tick now) {
    if (!process) throw std::invalid_argument("Cannot dispatch null process");
    if (!isIdle()) throw std::logic_error("Cannot dispatch to a busy CPU");
    process->dispatch(id_, now);
    currentProcess_ = process;
}

std::shared_ptr<Process> Cpu::release() {
    auto released = std::move(currentProcess_);
    return released;
}

void Cpu::executeOneTick() {
    if (isIdle()) return;
    currentProcess_->executeOneTick();
    ++busyTime_;
}

}  // namespace scheduler
