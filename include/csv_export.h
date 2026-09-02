#pragma once

#include "benchmark.h"

#include <fstream>
#include <string>
#include <vector>

namespace scheduler
{

    class CsvExporter
    {
    public:
        /**
         * Export benchmark results to a CSV file.
         *
         * @param results Vector of benchmark results
         * @param filename Output CSV file path
         */
        static void exportResults(
            const std::vector<BenchmarkResult> &results,
            const std::string &filename);

        /**
         * Export a single result row (useful for streaming results).
         */
        static std::string formatResultRow(const BenchmarkResult &result);

        /**
         * Get CSV header row.
         */
        static std::string getHeaderRow();

    private:
        static std::string escapeField(const std::string &field);
    };

} // namespace scheduler
