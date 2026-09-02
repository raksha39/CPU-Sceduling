#pragma once

#include "metrics.h"
#include "simulation.h"
#include "workload.h"

#include <memory>
#include <string>
#include <vector>

namespace scheduler
{

    /**
     * Complete metrics collection for a single benchmark experiment.
     * Captures all measurements taken during a simulation run.
     */
    struct BenchmarkResult
    {
        // Experiment parameters
        std::string algorithmName;
        std::string workloadType;
        CpuId cpuCount;
        std::size_t taskCount;

        // Timing metrics
        Tick simulationDuration;

        // Scheduling metrics
        double averageWaitingTime = 0.0;
        double averageTurnaroundTime = 0.0;
        double averageResponseTime = 0.0;
        std::uint64_t completedProcesses = 0;

        // Throughput
        double throughput = 0.0; // Processes completed per 1000 ticks

        // CPU utilization - calculated over entire simulation
        // Tracks cumulative CPU-seconds and divides by total possible CPU-seconds
        double cpuUtilization = 0.0;          // Percentage (0-100)
        std::uint64_t totalCpuBusyTicks = 0;  // Debug info: cumulative busy ticks across all CPUs
        std::uint64_t totalPossibleTicks = 0; // Debug info: total possible ticks (duration * cpuCount)

        // Migration metrics
        std::uint64_t contextSwitches = 0;
        std::uint64_t migrations = 0;
        Tick migrationOverhead = 0;

        // Load balancing metrics - calculated over entire simulation
        // Tracks average load imbalance across all time steps
        double avgLoadImbalance = 0.0;      // Average imbalance across time
        std::uint64_t maxLoadImbalance = 0; // Peak imbalance

        // Scheduling fairness metrics
        double maxWaitingTime = 0.0; // Longest wait time observed
        double minWaitingTime = 0.0; // Shortest wait time observed

        // Seed for reproducibility
        std::uint32_t randomSeed = 42;
    };

    class BenchmarkRunner
    {
    public:
        BenchmarkRunner();

        /**
         * Run a single benchmark experiment with the specified configuration.
         */
        [[nodiscard]] BenchmarkResult runExperiment(
            const std::string &algorithmName,
            std::unique_ptr<Scheduler> scheduler,
            WorkloadType workload,
            CpuId cpuCount,
            std::size_t taskCount,
            std::uint32_t seed = 42);

        /**
         * Run all benchmarks across algorithms, workloads, CPU counts, and task counts.
         * Returns a vector of all results.
         */
        [[nodiscard]] std::vector<BenchmarkResult> runAllBenchmarks(
            const std::vector<std::string> &algorithms,
            const std::vector<WorkloadType> &workloads,
            const std::vector<CpuId> &cpuCounts,
            const std::vector<std::size_t> &taskCounts,
            std::uint32_t seed = 42);

    private:
        [[nodiscard]] std::unique_ptr<Scheduler> createScheduler(const std::string &name);
    };

} // namespace scheduler
