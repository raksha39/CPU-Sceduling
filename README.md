# Multi-Core CPU Scheduler and User-Space Thread Runtime

An incremental C++20 scheduler simulator for Windows. It models scheduling decisions deterministically; it is not a kernel scheduler and does not control the Windows scheduler.

## Overview

This project implements a comprehensive multi-core CPU scheduler simulator with:

- Multiple scheduling algorithms (Round Robin, Priority, MLFQ, FCFS)
- Comprehensive benchmark framework (300 experiments)
- Deterministic simulation with reproducible results
- Full metrics collection and CSV export
- Cross-platform execution support

## Build

```bash
# Configure build
cmake -S . -B build

# Build all targets
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release --output-on-failure

# Run benchmark suite
./build/benchmark
```

GoogleTest is discovered through an installed CMake package. Configure with `-DGTest_DIR=<path>` when necessary.

## Running the Benchmark Suite

### Quick Start

```powershell
# Windows
.\scripts\run_benchmarks.ps1

# Linux/macOS
./scripts/run_benchmarks.sh

# Python (cross-platform)
python scripts/run_benchmarks.py
```

### What Gets Tested

The benchmark suite automatically evaluates:

- **3 Algorithms**: Round Robin, Priority, MLFQ
- **5 Workloads**: CPU-bound, I/O-bound, Mixed, Bursty, Random
- **5 CPU Counts**: 1, 2, 4, 8, 16 cores
- **4 Task Counts**: 1,000, 10,000, 50,000, 100,000

**Total: 300 experiments**

### Metrics Collected

Each experiment measures:

- Average waiting time, response time, turnaround time
- Throughput (processes/1000 ticks)
- CPU utilization (%)
- Context switches
- Process migrations
- Load imbalance (average and peak)
- Process scheduling fairness

### Results

Results are exported to `benchmark_results.csv` with 21 columns:

- Algorithm, WorkloadType, CPUCount, TaskCount
- Timing metrics (SimulationDuration, AverageWaitingTime, etc.)
- Performance metrics (Throughput, CPUUtilization, etc.)
- Load balancing metrics (Migrations, AvgLoadImbalance, MaxLoadImbalance)
- Fairness metrics (MaxWaitingTime, MinWaitingTime)

## Documentation

- **[BENCHMARKING.md](docs/BENCHMARKING.md)** - Complete methodology and interpretation guide
- **[BENCHMARK_QUICKSTART.md](docs/BENCHMARK_QUICKSTART.md)** - Quick start guide
- **[BENCHMARK_FRAMEWORK.md](docs/BENCHMARK_FRAMEWORK.md)** - Framework architecture
- **[REPRODUCIBLE_BENCHMARKS.md](docs/REPRODUCIBLE_BENCHMARKS.md)** - Detailed command reference
- **[RESULTS_ANALYSIS.md](docs/RESULTS_ANALYSIS.md)** - CSV analysis guide
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System architecture

## Key Features

✅ **Reproducible Results** - Deterministic with seed 42, same hardware = identical output  
✅ **No Fabricated Data** - All metrics derived from actual simulation runs  
✅ **Comprehensive Evaluation** - 300 experiments covering diverse scenarios  
✅ **Enhanced Metrics** - CPU utilization and load imbalance tracked over time  
✅ **Cross-Platform** - Windows, Linux, macOS support  
✅ **Easy Execution** - Single command to run full benchmark suite  
✅ **Professional Export** - RFC 4180 compliant CSV with proper escaping
