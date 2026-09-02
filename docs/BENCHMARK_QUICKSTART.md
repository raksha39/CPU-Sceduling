# Benchmark Framework Quick Start

## Overview

This benchmark framework provides a comprehensive evaluation of scheduling algorithms across diverse configurations and metrics.

**Total Benchmarks**: 300 experiments  
**Algorithms**: Round Robin, Priority, MLFQ  
**Workloads**: CPU-bound, I/O-bound, Mixed, Bursty, Random  
**CPUs**: 1, 2, 4, 8, 16  
**Task Counts**: 1,000, 10,000, 50,000, 100,000

## Quick Start

### Option 1: PowerShell (Windows)

```powershell
# Run with full output
.\scripts\run_benchmarks.ps1

# Skip build (if already compiled)
.\scripts\run_benchmarks.ps1 -SkipBuild

# Clean build
.\scripts\run_benchmarks.ps1 -Clean
```

### Option 2: Bash (Linux/macOS)

```bash
# Make script executable
chmod +x ./scripts/run_benchmarks.sh

# Run benchmarks
./scripts/run_benchmarks.sh

# Skip build
./scripts/run_benchmarks.sh --skip-build

# Clean build
./scripts/run_benchmarks.sh --clean
```

### Option 3: Python (Cross-platform)

```bash
# Run benchmarks
python scripts/run_benchmarks.py

# Python will auto-detect platform and handle build
```

## What Happens During Benchmark

1. **Build Phase** (2-5 minutes)
   - Configures CMake build system
   - Compiles benchmark executable
   - Verifies dependencies

2. **Benchmark Phase** (5-15 minutes)
   - Runs 300 individual experiments
   - Progress shown in real-time
   - No performance numbers fabricated

3. **Processing Phase**
   - Calculates aggregate metrics
   - Validates results
   - Exports to CSV

4. **Results**
   - `benchmark_YYYYMMDD_HHMMSS.csv` saved in `results/` directory
   - Metadata saved for reproducibility
   - Summary statistics printed

## Understanding Results

### CSV Columns

| Column                | Unit                 | Meaning                              |
| --------------------- | -------------------- | ------------------------------------ |
| Algorithm             | -                    | Scheduler algorithm name             |
| WorkloadType          | -                    | Workload pattern used                |
| CPUCount              | cores                | Number of CPUs simulated             |
| TaskCount             | processes            | Number of tasks in workload          |
| SimulationDuration    | ticks                | Total simulation time                |
| AverageWaitingTime    | ticks                | Avg time from arrival to dispatch    |
| AverageTurnaroundTime | ticks                | Avg time from arrival to completion  |
| AverageResponseTime   | ticks                | Avg time from arrival to first run   |
| CompletedProcesses    | count                | Total processes completed            |
| Throughput            | processes/1000 ticks | Scheduling throughput                |
| CPUUtilization        | %                    | CPU busy percentage                  |
| ContextSwitches       | count                | Total context switch events          |
| Migrations            | count                | Process migrations between CPUs      |
| MigrationOverhead     | ticks                | Total migration cost                 |
| AvgLoadImbalance      | %                    | Queue imbalance metric               |
| RandomSeed            | -                    | Seed for reproducibility (always 42) |

### Analysis Examples

**Find best algorithm for interactive workloads (low latency):**

```
Filter: WorkloadType = "io-bound"
Sort by: AverageWaitingTime (ascending)
Best: Algorithm with lowest waiting time
```

**Find best algorithm for throughput:**

```
Filter: CPUCount = 4 (typical system)
Sort by: Throughput (descending)
Best: Algorithm with highest throughput
```

**Analyze scalability:**

```
Filter: Algorithm = "round-robin", WorkloadType = "cpu-bound"
Sort by: CPUCount (ascending)
Plot: Throughput vs CPUCount
Expected: Near-linear improvement with more CPUs
```

## Reproducibility

All benchmarks use **seed 42** for deterministic results:

- Same hardware + OS → identical results
- Different hardware → same relative performance
- Multiple runs → zero variance

To verify reproducibility:

```bash
# Run twice
./scripts/run_benchmarks.ps1
./scripts/run_benchmarks.ps1

# Compare CSVs - should be identical
Compare-Object (Get-Content results\benchmark_*.csv) (Get-Content results\benchmark_*.csv)
```

## Interpreting Different Metrics

### When to Minimize Each Metric

| Metric           | Minimize For        | Reason                       |
| ---------------- | ------------------- | ---------------------------- |
| Waiting Time     | Interactive systems | Reduces user-perceived delay |
| Turnaround Time  | Batch processing    | Improves throughput          |
| Context Switches | Energy efficiency   | Reduces switching overhead   |
| Migrations       | Cache efficiency    | Preserves memory locality    |
| Load Imbalance   | Fairness            | Prevents CPU starvation      |

### When to Maximize Each Metric

| Metric              | Maximize For       | Reason             |
| ------------------- | ------------------ | ------------------ |
| Throughput          | Overall capacity   | More work per time |
| CPU Utilization     | Efficiency         | Less idle time     |
| Completed Processes | Workload finishing | Faster completion  |

## Common Patterns to Look For

### CPU-Bound Workloads

- **Round Robin**: Fair scheduling, moderate wait times
- **Priority**: Depends on priority assignment, starvation possible
- **MLFQ**: Adapts over time, good balance

### I/O-Bound Workloads

- **Priority**: Good if I/O tasks have high priority
- **MLFQ**: Excellent (auto-promotes I/O-bound tasks)
- **Round Robin**: Less optimal (fixed quantum)

### Scaling with CPU Count

- Linear speedup: 2 CPUs → 2× throughput (ideal)
- Sub-linear: Some serialization overhead
- Super-linear: Unlikely, indicates measurement artifact

## Troubleshooting

**Issue: Very high waiting times**

- Check if workload is realistic
- Verify process counts aren't too small
- See if algorithms are starving low-priority work

**Issue: Inconsistent migrations**

- Load balancing varies by algorithm
- Check if migrations are beneficial or harmful
- Compare cache-related metrics

**Issue: Zero context switches**

- Single CPU systems (CPUCount=1)
- All processes complete on first dispatch
- Check metrics make sense for workload

**Issue: Build fails**

- Verify CMake is installed
- Check C++20 compiler support
- See BENCHMARKING.md for compiler requirements

## Next Steps

1. **Open in Spreadsheet**
   - Excel/LibreOffice: File → Open → benchmark\_\*.csv
   - Create pivot tables by algorithm, workload

2. **Create Charts**
   - X-axis: CPUCount or TaskCount
   - Y-axis: Key metric (throughput, latency)
   - Series: Different algorithms
   - Compare performance visually

3. **Statistical Analysis**
   - Calculate averages by category
   - Find outliers or anomalies
   - Test for significant differences

4. **Report Writing**
   - Document algorithm differences
   - Explain why certain algorithms excel
   - Recommend for specific use cases

## Files Generated

```
results/
├── benchmark_20250902_123456.csv      # Main results
├── benchmark_20250902_234567.csv      # Previous run
└── metadata_20250902_123456.json      # Experiment metadata
```

## Further Reading

- [BENCHMARKING.md](../docs/BENCHMARKING.md) - Complete methodology documentation
- [ARCHITECTURE.md](../docs/ARCHITECTURE.md) - System design overview
- Source code: [benchmark.h](../include/benchmark.h), [workload.h](../include/workload.h)

## Performance Expectations

### Runtime by Configuration

- **Light** (1,000 tasks, 1 CPU): ~0.5 seconds
- **Medium** (10,000 tasks, 4 CPUs): ~2 seconds
- **Heavy** (100,000 tasks, 16 CPUs): ~10 seconds
- **Full Suite** (300 experiments): 5-15 minutes

### Typical Throughput

- CPU-bound: 100-500 processes/1000 ticks
- I/O-bound: 500-2000 processes/1000 ticks
- Mixed: 200-800 processes/1000 ticks

### Typical Wait Times

- CPU-bound: 50-300 ticks
- I/O-bound: 10-100 ticks
- Mixed: 30-200 ticks

## Contact & Support

For issues, questions, or contributions:

- Check [BENCHMARKING.md](../docs/BENCHMARKING.md) for detailed documentation
- Review sample results in [results/](../results/) directory
- See algorithm source in [schedulers/](../schedulers/) directory
