#pragma once

#include "process.h"

#include <deque>
#include <memory>

namespace scheduler {

class ReadyQueue {
public:
    void enqueue(std::shared_ptr<Process> process);
    [[nodiscard]] std::shared_ptr<Process> dequeueEligible(CpuId cpuId);
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::deque<std::shared_ptr<Process>> processes_;
};

}  // namespace scheduler
