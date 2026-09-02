#include "csv_export.h"

#include <iomanip>
#include <sstream>

namespace scheduler
{

    std::string CsvExporter::escapeField(const std::string &field)
    {
        // If field contains comma, quote, or newline, wrap in quotes and escape quotes
        if (field.find(',') != std::string::npos ||
            field.find('"') != std::string::npos ||
            field.find('\n') != std::string::npos)
        {
            std::string escaped = "\"";
            for (char c : field)
            {
                if (c == '"')
                {
                    escaped += "\"\""; // Escape quotes by doubling
                }
                else
                {
                    escaped += c;
                }
            }
            escaped += "\"";
            return escaped;
        }
        return field;
    }

    std::string CsvExporter::getHeaderRow()
    {
        return "Algorithm,WorkloadType,CPUCount,TaskCount,SimulationDuration,AverageWaitingTime,"
               "AverageTurnaroundTime,AverageResponseTime,CompletedProcesses,Throughput,"
               "CPUUtilization,ContextSwitches,Migrations,MigrationOverhead,AvgLoadImbalance,"
               "MaxLoadImbalance,TotalCpuBusyTicks,TotalPossibleTicks,MaxWaitingTime,"
               "MinWaitingTime,RandomSeed\n";
    }

    std::string CsvExporter::formatResultRow(const BenchmarkResult &result)
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(6);

        ss << escapeField(result.algorithmName) << ","
           << escapeField(result.workloadType) << ","
           << result.cpuCount << ","
           << result.taskCount << ","
           << result.simulationDuration << ","
           << result.averageWaitingTime << ","
           << result.averageTurnaroundTime << ","
           << result.averageResponseTime << ","
           << result.completedProcesses << ","
           << result.throughput << ","
           << result.cpuUtilization << ","
           << result.contextSwitches << ","
           << result.migrations << ","
           << result.migrationOverhead << ","
           << result.avgLoadImbalance << ","
           << result.maxLoadImbalance << ","
           << result.totalCpuBusyTicks << ","
           << result.totalPossibleTicks << ","
           << result.maxWaitingTime << ","
           << result.minWaitingTime << ","
           << result.randomSeed << "\n";

        return ss.str();
    }

    void CsvExporter::exportResults(
        const std::vector<BenchmarkResult> &results,
        const std::string &filename)
    {

        std::ofstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }

        // Write header
        file << getHeaderRow();

        // Write data rows
        for (const auto &result : results)
        {
            file << formatResultRow(result);
        }

        file.close();
    }

} // namespace scheduler
