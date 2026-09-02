#include "benchmark.h"
#include "scheduler.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <limits>

namespace scheduler
{

    BenchmarkRunner::BenchmarkRunner() = default;

    BenchmarkResult BenchmarkRunner::runExperiment(
        const std::string &algorithmName,
        std::unique_ptr<Scheduler> scheduler,
        WorkloadType workload,
        CpuId cpuCount,
        std::size_t taskCount,
        std::uint32_t seed)
    {

        BenchmarkResult result;
        result.algorithmName = algorithmName;
        result.workloadType = toString(workload);
        result.cpuCount = cpuCount;
        result.taskCount = taskCount;
        result.randomSeed = seed;

        // Generate workload
        WorkloadGenerator generator(seed);
        auto processes = generator.generate(workload, taskCount, cpuCount);

        // Create simulation
        SimulationEngine simulation(cpuCount, std::move(scheduler));

        // Add all processes
        for (auto &process : processes)
        {
            simulation.addProcess(process);
        }

        // Metrics tracking throughout simulation
        std::uint64_t totalLoadImbalance = 0;
        std::uint64_t imbalanceSamples = 0;

        // Run simulation to completion
        Tick maxTick = 100000; // Safety limit
        while (simulation.hasWork() && simulation.now() < maxTick)
        {
            // Track load imbalance at each tick
            Tick currentImbalance = simulation.loadImbalance();
            totalLoadImbalance += currentImbalance;
            imbalanceSamples++;
            if (currentImbalance > result.maxLoadImbalance)
            {
                result.maxLoadImbalance = currentImbalance;
            }

            // Track CPU busy ticks for this tick
            for (const auto &cpu : simulation.cpus())
            {
                if (!cpu.isIdle())
                {
                    result.totalCpuBusyTicks++;
                }
            }

            simulation.advanceOneTick();
        }

        result.simulationDuration = simulation.now();

        // Calculate CPU utilization from tracked data
        result.totalPossibleTicks = result.simulationDuration * cpuCount;
        if (result.totalPossibleTicks > 0)
        {
            result.cpuUtilization = (result.totalCpuBusyTicks * 100.0) / result.totalPossibleTicks;
        }

        // Calculate average load imbalance over time
        if (imbalanceSamples > 0)
        {
            result.avgLoadImbalance = static_cast<double>(totalLoadImbalance) / imbalanceSamples;
        }

        // Calculate metrics
        auto metrics = simulation.metrics();
        result.averageWaitingTime = metrics.averageWaitingTime;
        result.averageTurnaroundTime = metrics.averageTurnaroundTime;
        result.averageResponseTime = metrics.averageResponseTime;
        result.completedProcesses = metrics.completedProcesses;

        // Calculate throughput (processes per 1000 ticks)
        if (result.simulationDuration > 0)
        {
            result.throughput = (metrics.completedProcesses * 1000.0) / result.simulationDuration;
        }

        // Migration metrics
        result.migrations = simulation.migrationCount();
        result.migrationOverhead = simulation.migrationOverhead();

        // Count context switches from events
        result.contextSwitches = 0;
        for (const auto &event : simulation.events())
        {
            if (event.type == EventType::Preempt ||
                event.type == EventType::ContextSwitch)
            {
                result.contextSwitches++;
            }
        }

        return result;
    }

    std::vector<BenchmarkResult> BenchmarkRunner::runAllBenchmarks(
        const std::vector<std::string> &algorithms,
        const std::vector<WorkloadType> &workloads,
        const std::vector<CpuId> &cpuCounts,
        const std::vector<std::size_t> &taskCounts,
        std::uint32_t seed)
    {

        std::vector<BenchmarkResult> allResults;

        // Calculate total experiments
        std::size_t totalExperiments = algorithms.size() * workloads.size() *
                                       cpuCounts.size() * taskCounts.size();
        std::size_t currentExperiment = 0;

        std::cout << "Starting benchmark suite: " << totalExperiments << " experiments\n";

        for (const auto &algo : algorithms)
        {
            for (const auto &workload : workloads)
            {
                for (const auto cpuCount : cpuCounts)
                {
                    for (const auto taskCount : taskCounts)
                    {
                        currentExperiment++;
                        std::cout << "[" << currentExperiment << "/" << totalExperiments << "] "
                                  << "Running: " << algo << " on " << toString(workload)
                                  << " (" << cpuCount << " CPUs, " << taskCount << " tasks)...\n";

                        auto scheduler = createScheduler(algo);
                        auto result = runExperiment(algo, std::move(scheduler), workload,
                                                    cpuCount, taskCount, seed);
                        allResults.push_back(result);
                    }
                }
            }
        }

        std::cout << "Benchmark suite complete!\n";
        return allResults;
    }

    std::unique_ptr<Scheduler> BenchmarkRunner::createScheduler(const std::string &name)
    {
        if (name == "round-robin")
        {
            return std::make_unique<RoundRobinScheduler>(10); // 10-tick quantum
        }
        else if (name == "priority")
        {
            return std::make_unique<PriorityScheduler>(true);
        }
        else if (name == "mlfq")
        {
            return std::make_unique<MlfqScheduler>();
        }
        else if (name == "fcfs")
        {
            return std::make_unique<FcfsScheduler>();
        }
        throw std::runtime_error("Unknown scheduler: " + name);
    }

} // namespace scheduler
