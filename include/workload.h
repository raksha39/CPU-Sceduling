#pragma once

#include "process.h"

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace scheduler
{

    enum class WorkloadType
    {
        CpuBound,
        IoBound,
        Mixed,
        Bursty,
        Random
    };

    [[nodiscard]] std::string toString(WorkloadType type);

    class WorkloadGenerator
    {
    public:
        explicit WorkloadGenerator(std::uint32_t seed = 42);

        /**
         * Generates a workload with specified characteristics.
         *
         * @param type The workload type (CPU-bound, I/O-bound, mixed, bursty, random)
         * @param taskCount Number of processes to generate
         * @param cpuCount Number of CPUs (used to adjust process distribution)
         * @param minBurst Minimum burst time per process (ticks)
         * @param maxBurst Maximum burst time per process (ticks)
         * @param duration Total simulation duration (ticks) - affects inter-arrival times
         * @return Vector of generated processes with arrival times, burst times, and priorities
         */
        [[nodiscard]] std::vector<std::shared_ptr<Process>> generate(
            WorkloadType type,
            std::size_t taskCount,
            CpuId cpuCount,
            Tick minBurst = 10,
            Tick maxBurst = 100,
            Tick duration = 10000);

    private:
        std::mt19937_64 rng_;
        std::uint32_t seed_;

        [[nodiscard]] std::vector<std::shared_ptr<Process>> generateCpuBound(
            std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration);

        [[nodiscard]] std::vector<std::shared_ptr<Process>> generateIoBound(
            std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration);

        [[nodiscard]] std::vector<std::shared_ptr<Process>> generateMixed(
            std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration);

        [[nodiscard]] std::vector<std::shared_ptr<Process>> generateBursty(
            std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration);

        [[nodiscard]] std::vector<std::shared_ptr<Process>> generateRandom(
            std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration);
    };

} // namespace scheduler
