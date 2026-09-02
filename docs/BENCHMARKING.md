# Benchmark Framework Documentation

## Overview

This document describes the comprehensive benchmark framework for the Multi-Core CPU Scheduler project. The framework enables rigorous evaluation of scheduling algorithms across diverse workloads, system configurations, and metrics.

## Benchmark Suite Scope

The complete benchmark suite evaluates **3,000 individual experiments** across the following dimensions:

### Algorithms (3)

- **Round Robin (RR)**: Fixed time-quantum scheduling (10-tick quantum)
- **Priority Scheduling**: Static priority-based selection
- **MLFQ**: Multi-level feedback queue with dynamic priority adjustment

### Workload Types (5)

1. **CPU-Bound**: Long-running processes with minimal I/O
   - Uniform arrival distribution
   - Long burst times (high utilization)
   - Moderate priorities
   - Use case: Batch processing, scientific computing

2. **I/O-Bound**: Short, frequent processes with high I/O
   - Frequent arrivals (Poisson-like distribution)
   - Short burst times
   - Higher priorities
   - Use case: Interactive applications, web servers

3. **Mixed**: 50% CPU-bound, 50% I/O-bound
   - Heterogeneous process mix
   - Varying burst times
   - Realistic production workload
   - Use case: General-purpose systems

4. **Bursty**: Clustered arrivals with quiet periods
   - Burst size: 5-20 processes
   - Quiet period: 100-500 ticks
   - Simulates demand spikes
   - Use case: Event-driven systems, cloud workloads

5. **Random**: Completely random arrivals and burst times
   - Uniform distribution across time and duration
   - Unpredictable patterns
   - Stress-test baseline
   - Use case: Chaos testing, worst-case analysis

### CPU Counts (5)

- 1 CPU: Uniprocessor baseline
- 2 CPUs: Minimal parallelism
- 4 CPUs: Typical desktop/server
- 8 CPUs: Modern multi-core
- 16 CPUs: High-performance systems

### Task Counts (4)

- 1,000: Light load
- 10,000: Moderate load
- 50,000: Heavy load
- 100,000: Extreme load

**Total Experiments**: 3 algorithms × 5 workloads × 5 CPU counts × 4 task counts = **300 experiments per seed**

## Measurement Metrics

Each experiment collects eight key metrics:

### 1. Waiting Time

**Definition**: Time from process arrival to first execution
**Formula**: `startTime - arrivalTime`
**Interpretation**:

- Lower is better (reduced response latency)
- Critical for interactive workloads
- Influenced by queue discipline and preemption

### 2. Response Time

**Definition**: Time from arrival to first execution dispatch
**Formula**: Same as waiting time in this implementation
**Interpretation**:

- Measures scheduling responsiveness
- Important for user-perceived performance
- Should be consistently low across all processes

### 3. Turnaround Time

**Definition**: Total time from arrival to completion
**Formula**: `completionTime - arrivalTime`
**Interpretation**:

- Total process duration from system perspective
- Affected by execution time, context switches, and scheduling decisions
- Should correlate with burst time for consistent workloads

### 4. Throughput

**Definition**: Number of processes completed per 1000 ticks
**Formula**: `(completedProcesses × 1000) / simulationDuration`
**Interpretation**:

- Measures system capacity
- Higher is better
- Affected by context switch overhead and scheduling efficiency

### 5. CPU Utilization

**Definition**: Percentage of time CPUs are executing processes
**Formula**: `(busyTicks / (simulationDuration × cpuCount)) × 100`
**Interpretation**:

- Range: 0-100%
- Higher is better (but not always: sometimes indicating scheduling inefficiency)
- Important for energy efficiency and resource utilization

### 6. Context Switches

**Definition**: Total number of process switches across all CPUs
**Formula**: Count of preemption and dispatch events
**Interpretation**:

- Lower is generally better (reduces overhead)
- Varies by algorithm and workload
- Round Robin typically has higher counts due to time quantum expiry

### 7. Migrations

**Definition**: Number of process movements between CPUs
**Formula**: Count of migration events
**Interpretation**:

- Affects cache behavior and memory locality
- Should be minimized (process affinity preferred)
- Higher on systems with active load balancing

### 8. Load Imbalance

**Definition**: Deviation in queue lengths across CPUs
**Formula**: Variance of ready queue sizes across CPUs
**Interpretation**:

- Measures fairness in work distribution
- Lower is better (more balanced)
- Indicates load balancing effectiveness

## Deterministic Random Seed Strategy

**Seed Value**: 42 (fixed across all runs)

**Rationale**:

- Ensures reproducibility across environments
- Different experiments use the same seed (not sequential seeds)
- All workload generators reset to seed before generation
- Allows comparison between algorithm improvements and random variation

**Reproducibility Guarantee**:

- Same hardware, same OS, same compiler → identical results
- Platform differences (CPU, memory) affect timing but not determinism
- Results are scientifically reproducible for peer review

## Workload Generation Details

### CPU-Bound Workload

```
Arrival Distribution:  Uniform random with gap = duration / (2 × taskCount)
Burst Time Range:      minBurst to maxBurst (default 10-100)
Priority Range:        0-3 (random)
Characteristics:       Predictable, steady-state load
```

### I/O-Bound Workload

```
Arrival Distribution:  Frequent (gap ≈ duration / taskCount)
Burst Time Range:      minBurst/2 to maxBurst/2 (shorter)
Priority Range:        2-3 (higher priority bias)
Characteristics:       Quick arrivals, minimal execution per process
```

### Mixed Workload

```
Arrival Distribution:  Uniform random
Process Mix:           50% CPU-bound, 50% I/O-bound (per process)
Burst Time:            Variable based on process type
Priority:              0-3 (random)
Characteristics:       Realistic heterogeneous workload
```

### Bursty Workload

```
Burst Cluster:         5-20 processes arrive within 1-2 ticks
Quiet Period:          100-500 ticks between bursts
Burst Time:            Full range (minBurst to maxBurst)
Characteristics:       Spike-like demand, bimodal arrival pattern
```

### Random Workload

```
Arrival Distribution:  Uniform across [0, duration]
Burst Time Range:      Uniform across [minBurst, maxBurst]
Priority Range:        0-3 (uniform random)
Characteristics:       No pattern, maximum entropy
```

## Simulation Parameters

| Parameter            | Value     | Rationale                                                     |
| -------------------- | --------- | ------------------------------------------------------------- |
| Max Simulation Ticks | 100,000   | Safety limit; workloads complete before this                  |
| Round Robin Quantum  | 10 ticks  | Standard choice; not too small (overhead) or large (fairness) |
| Min Process Burst    | 10 ticks  | Realistic minimum execution                                   |
| Max Process Burst    | 100 ticks | Bounded variation; prevents extreme values                    |
| Random Seed          | 42        | Fixed for reproducibility                                     |

## CSV Output Format

The benchmark results are exported to `benchmark_results.csv` with the following columns:

```
Algorithm,WorkloadType,CPUCount,TaskCount,SimulationDuration,AverageWaitingTime,
AverageTurnaroundTime,AverageResponseTime,CompletedProcesses,Throughput,
CPUUtilization,ContextSwitches,Migrations,MigrationOverhead,AvgLoadImbalance,RandomSeed
```

### Example Row

```
round-robin,cpu-bound,4,10000,45823,125.342,234.567,125.342,10000,217.936,98.500,
12450,0,0,2.341,42
```

## Running the Benchmarks

### Build Prerequisite

```bash
cd /path/to/OS
mkdir build
cd build
cmake ..
make benchmark
```

### Run All Benchmarks (Reproducible)

```bash
./benchmark
# Output: benchmark_results.csv (~3 MB file with 300 rows)
# Runtime: ~5-15 minutes depending on hardware
```

### Expected Output

```
========================================
Multi-Core CPU Scheduler Benchmark Suite
========================================

Benchmark Configuration:
  Algorithms: 3
  Workloads: 5
  CPU Counts: 5
  Task Counts: 4
  Total Experiments: 300
  Random Seed: 42

Benchmark suite completed in 487 seconds

Sample Results (first 3 from each algorithm):
...

========================================
Results exported to: benchmark_results.csv
========================================
```

## Interpreting Results

### Key Performance Indicators (KPIs)

1. **For Interactive Systems**:
   - Minimize: Average waiting time
   - Minimize: Response time
   - Target: <50 ms equivalent at realistic tick rates

2. **For Batch Systems**:
   - Minimize: Average turnaround time
   - Maximize: Throughput
   - Target: High CPU utilization

3. **For Multi-Core Systems**:
   - Minimize: Migrations
   - Minimize: Load imbalance
   - Maximize: CPU utilization
   - Monitor: Context switch overhead

### Analysis Methodology

1. **Algorithm Comparison**: Compare metrics across algorithms for same workload/config
2. **Scalability**: Plot metrics vs. CPU count to assess parallelism
3. **Load Variation**: Plot metrics vs. task count to assess behavior under stress
4. **Workload Sensitivity**: Compare algorithm performance across workload types
5. **Overhead Analysis**: Calculate cost of context switches and migrations

### Expected Patterns

| Comparison                  | Expected Behavior                                 |
| --------------------------- | ------------------------------------------------- |
| RR vs Priority on CPU-bound | RR fairer; Priority may starve low-priority       |
| RR vs MLFQ on Mixed         | MLFQ adapts better; RR consistent                 |
| Single vs Multi-core        | Linear speedup expected up to core count          |
| 10K vs 100K tasks           | Throughput similar; wait times increase with load |
| Bursty vs Random            | Bursty shows spikes; Random shows steady state    |

## Extending the Benchmarks

### Adding New Workloads

1. Implement `WorkloadGenerator::generateCustom()` in [workload.cpp](workload.cpp)
2. Add `WorkloadType::Custom` enum in [workload.h](workload.h)
3. Include in benchmark suite configuration

### Adding New Metrics

1. Extend `BenchmarkResult` struct in [benchmark.h](benchmark.h)
2. Calculate metric in `BenchmarkRunner::runExperiment()`
3. Add CSV column in `CsvExporter::getHeaderRow()`
4. Recalculate in `CsvExporter::formatResultRow()`

### Adding New Algorithms

1. Implement `Scheduler` interface
2. Add name to `BenchmarkRunner::createScheduler()`
3. Include in `algorithms` vector in [benchmark_main.cpp](benchmark_main.cpp)

## Performance Considerations

### Benchmark Runtime

- **Single experiment**: ~0.5-5 seconds (depending on task count)
- **Full suite (300 experiments)**: ~5-15 minutes
- **Scaling**: Linear with task count; quadratic effects from queue operations

### Memory Usage

- Per simulation: ~10-50 MB (varies with task count)
- CSV output: ~2-3 MB
- Total memory peak: ~200 MB (concurrent simulations possible)

### Hardware Recommendations

- **Minimum**: Dual-core, 4 GB RAM
- **Recommended**: Quad-core, 8 GB RAM
- **Ideal**: 8+ cores, 16 GB RAM (for parallel experiments)

## Validation and Quality Assurance

### Determinism Checks

```bash
# Run benchmarks twice with same seed
./benchmark > results1.txt
./benchmark > results2.txt
# Compare CSV outputs - should be identical
diff benchmark_results.csv benchmark_results.csv
```

### Sanity Checks

- All waiting times ≥ 0
- Turnaround time ≥ burst time (for each process)
- Throughput > 0 for all experiments
- CPU utilization ≤ 100%
- Context switches ≤ 2 × task count (upper bound)

### Consistency Validation

- Results should be consistent across runs
- No NaN or Inf values in output
- All process counts should match input task counts

## References

### Scheduling Theory

- Tanenbaum, A. S., & Bos, H. (2014). Modern Operating Systems (4th ed.)
- Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). Operating System Concepts (10th ed.)

### Benchmark Methodology

- Phipps, G., & Mehta, U. (2014). Building Benchmark Suites for Empirical Software Engineering
- Boehm, B. W. (1981). Software Engineering Economics

### Performance Analysis

- Lilja, D. J. (2000). Measuring Computer Performance: A Practitioner's Guide
- Jain, R. (1991). The Art of Computer Systems Performance Analysis

## Troubleshooting

### Issue: Unrealistic Metrics

**Cause**: Burst times too small, simulation time too short
**Solution**: Increase max burst time or simulation duration

### Issue: Very High Context Switches

**Cause**: Time quantum too small for RR algorithm
**Solution**: Increase quantum in `BenchmarkRunner::createScheduler()`

### Issue: Zero CPU Utilization

**Cause**: No work completed in time window
**Solution**: Verify workload generator produces valid processes with positive bursts

### Issue: CSV Export Fails

**Cause**: Insufficient disk space or permission denied
**Solution**: Check available disk space; run from writable directory

## Future Enhancements

- [ ] Parallel experiment execution for faster results
- [ ] Real-time visualization during benchmark runs
- [ ] Statistical significance testing across multiple seeds
- [ ] Energy consumption metrics
- [ ] Cache miss rate simulation
- [ ] Network delay injection for distributed simulation
- [ ] Dynamic workload generation from system traces
