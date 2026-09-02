#include "benchmark.h"
#include "csv_export.h"
#include "workload.h"

#include <chrono>
#include <iostream>
#include <iomanip>

int main(int argc, char *argv[])
{
    using namespace scheduler;

    std::cout << "========================================\n"
              << "Multi-Core CPU Scheduler Benchmark Suite\n"
              << "========================================\n\n";

    // Benchmark configuration
    std::vector<std::string> algorithms = {
        "round-robin",
        "priority",
        "mlfq"};

    std::vector<WorkloadType> workloads = {
        WorkloadType::CpuBound,
        WorkloadType::IoBound,
        WorkloadType::Mixed,
        WorkloadType::Bursty,
        WorkloadType::Random};

    std::vector<CpuId> cpuCounts = {1, 2, 4, 8, 16};
    std::vector<std::size_t> taskCounts = {1000, 10000, 50000, 100000};

    std::uint32_t randomSeed = 42; // Fixed for reproducibility

    std::cout << "Benchmark Configuration:\n"
              << "  Algorithms: " << algorithms.size() << "\n"
              << "  Workloads: " << workloads.size() << "\n"
              << "  CPU Counts: " << cpuCounts.size() << "\n"
              << "  Task Counts: " << taskCounts.size() << "\n"
              << "  Total Experiments: "
              << (algorithms.size() * workloads.size() * cpuCounts.size() * taskCounts.size())
              << "\n"
              << "  Random Seed: " << randomSeed << "\n\n";

    auto startTime = std::chrono::high_resolution_clock::now();

    // Run benchmarks
    BenchmarkRunner runner;
    auto results = runner.runAllBenchmarks(
        algorithms, workloads, cpuCounts, taskCounts, randomSeed);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    std::cout << "\nBenchmark suite completed in " << duration.count() << " seconds\n";

    // Export results
    std::string outputFile = "benchmark_results.csv";
    std::cout << "Exporting results to: " << outputFile << "\n";
    CsvExporter::exportResults(results, outputFile);

    // Print summary statistics
    std::cout << "\n========================================\n"
              << "Results Summary\n"
              << "========================================\n\n";

    // Group results by algorithm and show top performers
    std::cout << "Sample Results (first 3 from each algorithm):\n\n";

    for (const auto &algo : algorithms)
    {
        int count = 0;
        std::cout << algo << ":\n";
        for (const auto &result : results)
        {
            if (result.algorithmName == algo && count < 3)
            {
                std::cout << "  - " << result.workloadType << " ("
                          << result.cpuCount << " CPUs, " << result.taskCount << " tasks): "
                          << "Avg Wait: " << std::fixed << std::setprecision(2)
                          << result.averageWaitingTime << " ticks\n";
                count++;
            }
        }
        std::cout << "\n";
    }

    std::cout << "========================================\n"
              << "Results exported to: " << outputFile << "\n"
              << "========================================\n";

    return 0;
}
