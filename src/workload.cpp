#include "workload.h"

#include <algorithm>
#include <stdexcept>

namespace scheduler
{

    std::string toString(WorkloadType type)
    {
        switch (type)
        {
        case WorkloadType::CpuBound:
            return "cpu-bound";
        case WorkloadType::IoBound:
            return "io-bound";
        case WorkloadType::Mixed:
            return "mixed";
        case WorkloadType::Bursty:
            return "bursty";
        case WorkloadType::Random:
            return "random";
        }
        throw std::runtime_error("Unknown workload type");
    }

    WorkloadGenerator::WorkloadGenerator(std::uint32_t seed)
        : rng_(seed), seed_(seed) {}

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generate(
        WorkloadType type,
        std::size_t taskCount,
        CpuId cpuCount,
        Tick minBurst,
        Tick maxBurst,
        Tick duration)
    {

        // Reset RNG with seed for deterministic generation
        rng_.seed(seed_);

        switch (type)
        {
        case WorkloadType::CpuBound:
            return generateCpuBound(taskCount, cpuCount, minBurst, maxBurst, duration);
        case WorkloadType::IoBound:
            return generateIoBound(taskCount, cpuCount, minBurst, maxBurst, duration);
        case WorkloadType::Mixed:
            return generateMixed(taskCount, cpuCount, minBurst, maxBurst, duration);
        case WorkloadType::Bursty:
            return generateBursty(taskCount, cpuCount, minBurst, maxBurst, duration);
        case WorkloadType::Random:
            return generateRandom(taskCount, cpuCount, minBurst, maxBurst, duration);
        }
        throw std::runtime_error("Unknown workload type");
    }

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generateCpuBound(
        std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration)
    {

        std::vector<std::shared_ptr<Process>> processes;
        processes.reserve(taskCount);

        // CPU-bound: tasks arrive uniformly distributed, long burst times
        std::uniform_int_distribution<Tick> burstDist(minBurst, maxBurst);
        std::uniform_int_distribution<Tick> arrivalDist(0, duration / static_cast<Tick>(taskCount) * 2);
        std::uniform_int_distribution<int> priorityDist(0, 3);

        Tick currentTime = 0;
        for (std::size_t i = 0; i < taskCount; ++i)
        {
            currentTime += arrivalDist(rng_);
            if (currentTime > duration)
                currentTime = duration;

            auto process = std::make_shared<Process>(
                static_cast<ProcessId>(i),
                currentTime,
                burstDist(rng_),
                priorityDist(rng_));
            processes.push_back(process);
        }

        return processes;
    }

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generateIoBound(
        std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration)
    {

        std::vector<std::shared_ptr<Process>> processes;
        processes.reserve(taskCount);

        // I/O-bound: more frequent arrivals, shorter burst times, higher priority
        Tick avgBurst = (minBurst + maxBurst) / 2;
        std::uniform_int_distribution<Tick> burstDist(minBurst / 2, avgBurst / 2);
        std::uniform_int_distribution<Tick> arrivalDist(1, duration / static_cast<Tick>(taskCount) + 1);
        std::uniform_int_distribution<int> priorityDist(2, 3); // Higher priority

        Tick currentTime = 0;
        for (std::size_t i = 0; i < taskCount; ++i)
        {
            currentTime += arrivalDist(rng_);
            if (currentTime > duration)
                currentTime = duration;

            auto process = std::make_shared<Process>(
                static_cast<ProcessId>(i),
                currentTime,
                burstDist(rng_),
                priorityDist(rng_));
            processes.push_back(process);
        }

        return processes;
    }

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generateMixed(
        std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration)
    {

        std::vector<std::shared_ptr<Process>> processes;
        processes.reserve(taskCount);

        // Mixed: 50% CPU-bound, 50% I/O-bound
        std::uniform_int_distribution<Tick> arrivalDist(0, duration / static_cast<Tick>(taskCount) * 2);
        std::uniform_int_distribution<int> priorityDist(0, 3);
        std::uniform_real_distribution<double> typeDist(0.0, 1.0);

        Tick currentTime = 0;
        for (std::size_t i = 0; i < taskCount; ++i)
        {
            currentTime += arrivalDist(rng_);
            if (currentTime > duration)
                currentTime = duration;

            bool isCpuBound = typeDist(rng_) < 0.5;
            Tick burst;
            if (isCpuBound)
            {
                std::uniform_int_distribution<Tick> cpuBurstDist(minBurst, maxBurst);
                burst = cpuBurstDist(rng_);
            }
            else
            {
                Tick avgBurst = (minBurst + maxBurst) / 2;
                std::uniform_int_distribution<Tick> ioBurstDist(minBurst / 2, avgBurst / 2);
                burst = ioBurstDist(rng_);
            }

            auto process = std::make_shared<Process>(
                static_cast<ProcessId>(i),
                currentTime,
                burst,
                priorityDist(rng_));
            processes.push_back(process);
        }

        return processes;
    }

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generateBursty(
        std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration)
    {

        std::vector<std::shared_ptr<Process>> processes;
        processes.reserve(taskCount);

        // Bursty: tasks arrive in clusters (bursts) followed by quiet periods
        std::uniform_int_distribution<Tick> burstDist(minBurst, maxBurst);
        std::uniform_int_distribution<Tick> burstSizeDist(5, 20);
        std::uniform_int_distribution<Tick> quietPeriodDist(100, 500);
        std::uniform_int_distribution<int> priorityDist(0, 3);

        Tick currentTime = 0;
        std::size_t generated = 0;

        while (generated < taskCount)
        {
            // Generate a burst of tasks
            Tick burstSize = burstSizeDist(rng_);
            for (Tick j = 0; j < burstSize && generated < taskCount; ++j, ++generated)
            {
                auto process = std::make_shared<Process>(
                    static_cast<ProcessId>(generated),
                    currentTime,
                    burstDist(rng_),
                    priorityDist(rng_));
                processes.push_back(process);
                currentTime += 1; // Small gap within burst
            }

            // Add quiet period
            currentTime += quietPeriodDist(rng_);
            if (currentTime > duration)
            {
                break;
            }
        }

        return processes;
    }

    std::vector<std::shared_ptr<Process>> WorkloadGenerator::generateRandom(
        std::size_t taskCount, CpuId cpuCount, Tick minBurst, Tick maxBurst, Tick duration)
    {

        std::vector<std::shared_ptr<Process>> processes;
        processes.reserve(taskCount);

        // Random: completely random arrival times and burst times with uniform distribution
        std::uniform_int_distribution<Tick> burstDist(minBurst, maxBurst);
        std::uniform_int_distribution<Tick> arrivalDist(0, duration);
        std::uniform_int_distribution<int> priorityDist(0, 3);

        for (std::size_t i = 0; i < taskCount; ++i)
        {
            auto process = std::make_shared<Process>(
                static_cast<ProcessId>(i),
                arrivalDist(rng_),
                burstDist(rng_),
                priorityDist(rng_));
            processes.push_back(process);
        }

        // Sort by arrival time to ensure proper event ordering
        std::sort(processes.begin(), processes.end(),
                  [](const auto &a, const auto &b)
                  { return a->arrivalTime() < b->arrivalTime(); });

        return processes;
    }

} // namespace scheduler
