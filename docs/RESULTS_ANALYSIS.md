# Benchmark Results Analysis Guide

## CSV Structure and Interpretation

Each row in `benchmark_results.csv` represents one complete experiment with 300 rows total.

### Column Definitions

#### Experiment Parameters (Read-Only)

```
Algorithm:      Scheduler used (round-robin, priority, mlfq)
WorkloadType:   Task pattern (cpu-bound, io-bound, mixed, bursty, random)
CPUCount:       Number of simulated CPUs (1, 2, 4, 8, 16)
TaskCount:      Number of tasks generated (1000, 10000, 50000, 100000)
RandomSeed:     Deterministic seed for reproducibility (always 42)
```

#### Simulation Results

```
SimulationDuration: Total ticks from start to all tasks complete
                   (typically 10,000-100,000 ticks depending on workload)
```

#### Scheduling Metrics (Lower is Better)

```
AverageWaitingTime:     Ticks from arrival to first execution
                        - Indicates scheduling responsiveness
                        - Critical for interactive workloads
                        - Expected range: 10-500 ticks

AverageResponseTime:    Same as waiting time in this implementation
                        - First time task gets CPU
                        - Should be minimized for user experience

AverageTurnaroundTime:  Ticks from arrival to completion
                        - Total time in system
                        - Includes execution + scheduling delays
                        - Expected range: 20-1000 ticks
```

#### Throughput Metrics (Higher is Better)

```
CompletedProcesses:     Total tasks that finished (should equal TaskCount)
                        - Validation metric
                        - If < TaskCount, simulation timed out

Throughput:             Processes completed per 1000 ticks
                        - Key performance indicator
                        - Typical range: 50-5000 proc/1000 ticks
                        - Formula: (CompletedProcesses × 1000) / SimulationDuration
```

#### Resource Utilization Metrics

```
CPUUtilization:         Percentage of available CPU cycles used
                        - Range: 0-100%
                        - Higher generally better
                        - May be < 100% due to synchronization
                        - May show > 100% due to measurement technique

ContextSwitches:        Number of process switches across all CPUs
                        - Lower is generally better (reduces overhead)
                        - Round Robin typically has most switches
                        - MLFQ and Priority typically have fewer
```

#### Multicore Specific Metrics

```
Migrations:             Number of times a process moved between CPUs
                        - Affects cache efficiency
                        - Lower is better (preserve locality)
                        - MLFQ typically has most migrations

MigrationOverhead:      Total simulated cost of migrations (ticks)
                        - Cumulative migration latency
                        - Only non-zero if load balancing enabled

AvgLoadImbalance:       Queue length variance across CPUs
                        - Indicates fairness of load distribution
                        - Lower is better (more balanced)
                        - Depends on balancing algorithm
```

## Quick Analysis Queries

### Query 1: Best Algorithm for Low Latency

```sql
SELECT Algorithm, AVG(AverageWaitingTime) as AvgWait
FROM results
WHERE WorkloadType = 'io-bound'
GROUP BY Algorithm
ORDER BY AvgWait ASC
LIMIT 1
```

**Expected Winner**: MLFQ (adapts to I/O patterns)

### Query 2: Best Throughput

```sql
SELECT Algorithm, AVG(Throughput) as AvgThroughput
FROM results
GROUP BY Algorithm
ORDER BY AvgThroughput DESC
```

**Expected Order**: Varies by workload; Priority good for uniform priority, MLFQ good for mixed

### Query 3: Scalability Analysis

```sql
SELECT CPUCount, Algorithm, AVG(Throughput) as AvgThroughput
FROM results
WHERE WorkloadType = 'cpu-bound'
GROUP BY CPUCount, Algorithm
ORDER BY Algorithm, CPUCount
```

**Expected Pattern**: Linear improvement (2 CPUs = 2× throughput, 4 CPUs = 4×, etc.)

### Query 4: Fairness Analysis

```sql
SELECT Algorithm, AVG(AvgLoadImbalance) as AvgImbalance
FROM results
WHERE CPUCount > 1
GROUP BY Algorithm
ORDER BY AvgImbalance ASC
```

**Expected Winner**: Round Robin (fairest); MLFQ may be less balanced

### Query 5: Overhead Analysis

```sql
SELECT Algorithm, AVG(ContextSwitches) as AvgSwitches
FROM results
GROUP BY Algorithm
ORDER BY AvgSwitches DESC
```

**Expected Order**:

1. Round Robin (highest - time quantum expiry)
2. MLFQ (medium - demotion events)
3. Priority (lowest - no forced switches)

## Workload-Specific Insights

### CPU-Bound Workloads

**Characteristics**:

- Long-running processes
- Minimal I/O
- Prefer consistency

**Analysis Focus**:

- Throughput (how much work done)
- Turnaround time (total completion time)
- Context switch efficiency

**Expected Results**:
| Metric | Best Algorithm |
|--------|----------------|
| Throughput | MLFQ ≥ Priority ≥ Round Robin |
| Waiting Time | Priority ≥ MLFQ ≥ Round Robin |
| Context Switches | Priority < MLFQ < Round Robin |

### I/O-Bound Workloads

**Characteristics**:

- Quick execution bursts
- Frequent I/O operations
- Benefit from fast scheduling

**Analysis Focus**:

- Waiting time (response latency)
- Responsiveness (first dispatch speed)
- CPU utilization (overlap I/O with other tasks)

**Expected Results**:
| Metric | Best Algorithm |
|--------|----------------|
| Waiting Time | MLFQ < Priority < Round Robin |
| Throughput | MLFQ > Round Robin > Priority |
| Context Switches | MLFQ > Round Robin > Priority |

### Mixed Workloads

**Characteristics**:

- 50% CPU-bound, 50% I/O-bound
- Realistic production scenario
- Tests adaptability

**Analysis Focus**:

- Balanced performance across metrics
- Fairness between process types
- Adaptive behavior

**Expected Results**:

- MLFQ should outperform by adapting
- Priority may starve some processes
- Round Robin should provide fairness

### Bursty Workloads

**Characteristics**:

- Clustered arrivals
- Quiet periods
- Spike handling

**Analysis Focus**:

- Queue management during spikes
- Behavior during quiet periods
- Overall load factor variation

**Expected Results**:

- MLFQ adapts well to bursts
- Priority depends on spike priority
- Round Robin maintains fairness

### Random Workloads

**Characteristics**:

- No predictable pattern
- Maximum entropy
- Stress testing

**Analysis Focus**:

- Consistency across runs
- Robustness to unpredictability
- No degradation from patterns

**Expected Results**:

- All algorithms should perform consistently
- No significant pattern advantages
- Good baseline for comparison

## Anomaly Detection

### Red Flag: Very High Waiting Times

**Possible Causes**:

- Runaway high-priority process
- Starvation of low-priority tasks
- Simulation scheduling issue

**Validation**:

- Check CompletedProcesses < TaskCount
- Look for specific algorithms with spikes
- Compare against baseline

### Red Flag: Zero Context Switches

**Possible Causes**:

- Single CPU (CPUCount = 1)
- All tasks complete immediately
- Measurement error

**Validation**:

- Should be expected for single-CPU
- Review SimulationDuration
- Check AverageTurnaroundTime

### Red Flag: CPU Utilization > 100%

**Possible Causes**:

- Measurement artifact
- Concurrent multithreading simulation
- Calculation error

**Validation**:

- Review CSV format
- Check CPU math: busyTicks / (duration × cpuCount)
- Reasonable max ~95% (overhead)

### Red Flag: Negative Values

**Should Never Happen**:

- Any negative value indicates bug
- Check workload generation
- Verify simulation completion

## Comparative Analysis

### Algorithm Comparison Matrix

Create a table comparing algorithms on same workload/config:

```
Algorithm       | Throughput | Wait Time | Context Sw | Migrations
Round Robin     |    450     |   125     |    1250    |     0
Priority        |    480     |   110     |     800    |     0
MLFQ            |    510     |    95     |    1100    |    50
```

**Reading**: MLFQ best throughput, lowest wait, but more migrations

### Scalability Curve

For each algorithm, plot throughput vs CPUCount:

```
Throughput (proc/1k ticks)
1000 |
     |     MLFQ
     |    /
     |   / Round Robin
800  |  /  /
     | /  / Priority
     |/  /
600  |--/
     |
400  |__________
1    2    4    8   16  CPUCount
```

**Reading**:

- Linear scaling = good parallelism
- Flattening = contention/synchronization issues
- Crossing = algorithm advantage changes with scale

## Statistical Measures

### Coefficient of Variation

Measures consistency across conditions:

```
CV = StdDev / Mean
- Low CV (<0.1): Consistent algorithm
- High CV (>0.3): Inconsistent, workload-dependent
```

### Speedup Factor

Measure improvement with more CPUs:

```
Speedup(n) = Throughput(n CPUs) / Throughput(1 CPU)
- Ideal: Speedup = n
- Typical: Speedup ≈ 0.7n to 0.9n
```

### Fairness Index (Jain's Index)

Measures task completion fairness:

```
Fairness = (Sum Waiting_i)² / (n × Sum Waiting_i²)
- Perfect fairness: 1.0
- Perfect unfairness: 1/n
```

## Export and Further Analysis

### Convert to Other Formats

```powershell
# CSV → Excel
Import-Csv results.csv | Export-Excel results.xlsx

# CSV → JSON
Get-Content results.csv | ConvertFrom-Csv | ConvertTo-Json | Out-File results.json

# CSV → SQL Database
Import-Csv results.csv | ForEach-Object { Insert into SQLite }
```

### Python Analysis

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('benchmark_results.csv')

# Plot throughput by algorithm
df.groupby('Algorithm')['Throughput'].mean().plot(kind='bar')
plt.show()

# Filter and analyze
io_bound = df[df['WorkloadType'] == 'io-bound']
print(io_bound.groupby('Algorithm')['AverageWaitingTime'].describe())
```

## Validation Checklist

Before drawing conclusions, verify:

- [ ] CompletedProcesses = TaskCount (no timeouts)
- [ ] SimulationDuration > 0 (simulation ran)
- [ ] No negative metrics (no bugs)
- [ ] AverageTurnaroundTime ≥ AverageWaitingTime
- [ ] AverageTurnaroundTime ≥ MaxBurst (fundamental limit)
- [ ] CPUUtilization ≤ 100% (physical constraint)
- [ ] ContextSwitches ≤ 2 × TaskCount (upper bound)
- [ ] Results repeatable (run twice, compare)

## Common Insights

### Why Round Robin Wins on Fairness

- Fixed time quantum ensures equal CPU access
- All processes get turns regularly
- No starvation of low-priority work

### Why MLFQ Wins on Adaptability

- Adjusts to I/O vs CPU-bound behavior
- Promotes fast I/O tasks
- Demotes CPU hogs
- Best overall for mixed workloads

### Why Priority Can Starve

- High-priority tasks may never yield
- Low-priority work waits indefinitely
- Good when priorities assigned correctly

### Why Throughput Improves with CPUs

- More parallel execution
- Less contention for scheduling
- Better overall cache utilization

## Making Recommendations

**For Interactive Systems** (web servers, UI apps):
→ Choose: **MLFQ** (best latency) or **Round Robin** (fair)
→ Metric to Minimize: **AverageWaitingTime**

**For Batch Processing** (scientific computing):
→ Choose: **Priority** (with careful priority assignment) or **MLFQ**
→ Metric to Minimize: **AverageTurnaroundTime**

**For Fair Resource Sharing**:
→ Choose: **Round Robin** (perfectly fair)
→ Metric to Monitor: **AvgLoadImbalance**

**For Energy Efficiency**:
→ Choose: **Priority** (fewer context switches)
→ Metric to Minimize: **ContextSwitches**

**For Multi-Core Systems**:
→ Choose: **MLFQ** (balances, adapts)
→ Metric to Track: **CPUUtilization** and **Migrations**
