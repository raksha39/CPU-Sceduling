# Reproducible Benchmark Commands Guide

This document provides comprehensive commands for running the benchmark suite in a reproducible manner with verification procedures.

## Quick Reference

| Purpose              | Command                            | Platform    |
| -------------------- | ---------------------------------- | ----------- |
| Full benchmark suite | `./scripts/run_benchmarks.ps1`     | Windows     |
| Full benchmark suite | `./scripts/run_benchmarks.sh`      | Linux/macOS |
| Cross-platform       | `python scripts/run_benchmarks.py` | Any         |
| Manual build & run   | See "Manual Execution" below       | Any         |

## Prerequisites

Before running benchmarks, verify your system has:

```bash
# Check CMake
cmake --version              # 3.20+ required

# Check C++ compiler
g++ --version               # Linux/macOS
cl.exe /?                   # Windows MSVC
clang++ --version          # Clang variant

# Check Python (for Python runner)
python3 --version          # 3.7+
```

## Platform-Specific Execution

### Windows (PowerShell)

#### Full Benchmark Suite

```powershell
# Navigate to project
cd C:\path\to\OS

# Run complete benchmark (300 experiments)
.\scripts\run_benchmarks.ps1

# Expected output:
# - Build phase: 2-5 minutes
# - Benchmark phase: 5-15 minutes
# - Results saved: results\benchmark_YYYYMMDD_HHMMSS.csv
```

#### With Options

```powershell
# Skip build step (if already compiled)
.\scripts\run_benchmarks.ps1 -SkipBuild

# Clean and rebuild
.\scripts\run_benchmarks.ps1 -Clean

# Specify project root
.\scripts\run_benchmarks.ps1 -ProjectRoot "D:\MyProject"
```

#### Verify Build

```powershell
# Check build directory
Get-ChildItem build\bin\
# Should show: benchmark.exe, scheduler.exe, scheduler_tests.exe

# Verify executable
Get-Command .\build\bin\benchmark.exe
```

#### Run Benchmark Only (Manual)

```powershell
# If already built:
cd build
.\benchmark.exe
# Results: benchmark_results.csv in current directory
```

### Linux/macOS (Bash)

#### Full Benchmark Suite

```bash
# Navigate to project
cd /path/to/OS

# Make scripts executable
chmod +x ./scripts/run_benchmarks.sh

# Run complete benchmark
./scripts/run_benchmarks.sh

# Expected output:
# - Build phase: 2-5 minutes
# - Benchmark phase: 5-15 minutes
# - Results saved: results/benchmark_YYYYMMDD_HHMMSS.csv
```

#### With Options

```bash
# Skip build
./scripts/run_benchmarks.sh --skip-build

# Clean build
./scripts/run_benchmarks.sh --clean

# Help
./scripts/run_benchmarks.sh --help
```

#### Verify Build

```bash
# Check executables
ls -la build/bin/
# Should show: benchmark, scheduler, scheduler_tests

# Verify executable
file ./build/bin/benchmark
```

#### Run Benchmark Only (Manual)

```bash
# If already built:
cd build
./benchmark
# Results: benchmark_results.csv in current directory
```

### Python (Cross-Platform)

#### Full Benchmark Suite

```bash
# Navigate to project
cd /path/to/OS

# Run benchmarks
python scripts/run_benchmarks.py

# Or
python3 scripts/run_benchmarks.py

# On Windows (if Python in PATH):
python scripts\run_benchmarks.py
```

#### With Options (in Python runner)

The Python runner supports:

- Automatic CMake configuration
- Platform detection
- Parallel build (if available)
- CSV validation
- Metadata archival

```bash
# Run with verbose output
python scripts/run_benchmarks.py 2>&1 | tee benchmark.log

# Run and check exit code
python scripts/run_benchmarks.py
echo "Exit code: $?"  # Linux/macOS
echo "Exit code: %ERRORLEVEL%"  # Windows
```

## Manual Execution

For maximum control, execute steps manually:

### Step 1: Configure Build

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake -S .. -B .              # Default (Debug mode)
cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release  # Release optimized
cmake -S .. -B . -G "Ninja"   # Use Ninja build system (if available)
```

### Step 2: Compile Benchmark

```bash
# Using make (Linux/macOS)
cmake --build . --config Release

# Using make with parallel jobs
cmake --build . --config Release -- -j4

# Using Ninja
cmake --build . --config Release -j4

# Using MSVC (Windows)
cmake --build . --config Release -- /m:4
```

### Step 3: Verify Build

```bash
# Check if executable exists
ls -la bin/benchmark              # Linux/macOS
dir bin\benchmark.exe             # Windows

# Quick test
./bin/benchmark --help 2>/dev/null || ./bin/benchmark | head -5
```

### Step 4: Run Benchmark

```bash
# Run benchmark
./bin/benchmark

# Capture output
./bin/benchmark 2>&1 | tee benchmark_output.log

# Run and time it
time ./bin/benchmark
/usr/bin/time -v ./bin/benchmark  # Detailed timing info
```

### Step 5: Archive Results

```bash
# Create results directory
mkdir -p results

# Move CSV to results with timestamp
mv benchmark_results.csv results/benchmark_$(date +%Y%m%d_%H%M%S).csv

# On Windows (PowerShell)
Move-Item benchmark_results.csv results/benchmark_$(Get-Date -Format 'yyyyMMdd_HHmmss').csv
```

## Reproducibility Verification

### Verify Results are Deterministic

Run benchmarks twice and compare:

```bash
# Run 1
./scripts/run_benchmarks.sh
# Save results
cp results/benchmark_*.csv benchmark_run1.csv

# Run 2
./scripts/run_benchmarks.sh
# Save results
cp results/benchmark_*.csv benchmark_run2.csv

# Compare (should be identical)
diff benchmark_run1.csv benchmark_run2.csv
# Output: (no differences - files are identical)
```

### Windows PowerShell Comparison

```powershell
# Run benchmarks twice
.\scripts\run_benchmarks.ps1
$file1 = Get-Item results\benchmark_*.csv | Sort-Object LastWriteTime -Descending | Select-Object -First 1

Start-Sleep -Seconds 5  # Wait a bit

.\scripts\run_benchmarks.ps1
$file2 = Get-Item results\benchmark_*.csv | Sort-Object LastWriteTime -Descending | Select-Object -First 1

# Compare files
$diff = Compare-Object (Get-Content $file1.FullName) (Get-Content $file2.FullName)
if ($diff) {
    Write-Host "FILES DIFFER - Reproducibility check FAILED"
    $diff | Select-Object -First 10
} else {
    Write-Host "FILES IDENTICAL - Reproducibility verified"
}
```

### Verify Specific Metrics

```bash
# Count experiments (should be 301 = 300 + 1 header)
wc -l results/benchmark_*.csv
# Expected: 301

# Check seed (should all be 42)
cut -d',' -f22 results/benchmark_*.csv | sort | uniq
# Expected: RandomSeed\n42
```

## Advanced Execution Scenarios

### Benchmark with Performance Monitoring

#### Linux with perf

```bash
# Install perf (if needed)
# Ubuntu: sudo apt-get install linux-tools

# Run with performance counters
perf stat ./build/bin/benchmark

# Run with detailed profiling
perf record ./build/bin/benchmark
perf report
```

#### Windows with Performance Monitor

```powershell
# Start performance monitor
perfmon.exe

# Add counters:
# - % Processor Time
# - Private Bytes
# - Context Switches/sec

# Start recording, run:
.\build\bin\benchmark.exe
```

### Benchmark with Memory Profiling

```bash
# Linux/macOS with Valgrind (if available)
valgrind --tool=massif ./build/bin/benchmark
ms_print massif.out.* | head -50

# macOS with Instruments (GUI)
instruments -t "System Trace" ./build/bin/benchmark
```

### Continuous Integration / CI/CD

#### GitHub Actions Example

```yaml
name: Benchmark Suite

on: [push]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install cmake g++
      - name: Build and Benchmark
        run: ./scripts/run_benchmarks.sh
      - name: Archive Results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: results/benchmark_*.csv
```

#### Local CI Simulation

```bash
# Build clean, run benchmarks, validate results
set -e

echo "=== Clean Build ==="
rm -rf build
mkdir build && cd build
cmake -S .. -B .
cmake --build . --config Release

echo "=== Running Benchmarks ==="
cd ..
./scripts/run_benchmarks.sh --skip-build

echo "=== Validating Results ==="
if [ -f results/benchmark_*.csv ]; then
    LINE_COUNT=$(wc -l < results/benchmark_*.csv)
    if [ "$LINE_COUNT" -eq 301 ]; then
        echo "✓ PASS: Correct number of experiments"
    else
        echo "✗ FAIL: Expected 301 lines, got $LINE_COUNT"
        exit 1
    fi
else
    echo "✗ FAIL: No results file found"
    exit 1
fi

echo "=== Benchmark Suite Completed Successfully ==="
```

## Expected Output

### Console Output

```
================================================================================
MULTI-CORE CPU SCHEDULER - REPRODUCIBLE BENCHMARK SUITE
================================================================================
[INFO] Project Root: /path/to/OS
[INFO] Build Directory: /path/to/OS/build
[INFO] Results Directory: /path/to/OS/results
[INFO] Timestamp: 20240902_143022

================================================================================
STEP 1: Building Benchmark Executable
================================================================================
[INFO] Configuring build...
[INFO] Building benchmark executable...
[SUCCESS] Benchmark executable built successfully

================================================================================
STEP 2: Running Benchmark Suite
================================================================================
[INFO] Starting 300 experiments:
[INFO]   3 algorithms × 5 workloads × 5 CPU counts × 4 task counts
[WARNING] This may take 5-15 minutes depending on hardware...

Starting benchmark suite: 300 experiments
[1/300] Running: round-robin on cpu-bound (1 CPUs, 1000 tasks)...
[2/300] Running: round-robin on cpu-bound (1 CPUs, 10000 tasks)...
[3/300] Running: round-robin on cpu-bound (1 CPUs, 50000 tasks)...
...
Benchmark suite complete!

[SUCCESS] Benchmarks completed in 487 seconds

================================================================================
STEP 3: Processing Results
================================================================================
[SUCCESS] Read 300 experiment results
[SUCCESS] Correct number of experiments (300)
[SUCCESS] Results archived to: results/benchmark_20240902_143022.csv

========================================
Results Summary
========================================

Sample Results (first 3 from each algorithm):

round-robin:
  - cpu-bound (1 CPUs, 1000 tasks): Avg Wait: 45.23 ticks
  - cpu-bound (2 CPUs, 1000 tasks): Avg Wait: 23.41 ticks
  - cpu-bound (4 CPUs, 1000 tasks): Avg Wait: 12.67 ticks

priority:
  - cpu-bound (1 CPUs, 1000 tasks): Avg Wait: 42.18 ticks
  - cpu-bound (2 CPUs, 1000 tasks): Avg Wait: 21.34 ticks
  - cpu-bound (4 CPUs, 1000 tasks): Avg Wait: 11.23 ticks

mlfq:
  - cpu-bound (1 CPUs, 1000 tasks): Avg Wait: 38.92 ticks
  - cpu-bound (2 CPUs, 1000 tasks): Avg Wait: 19.87 ticks
  - cpu-bound (4 CPUs, 1000 tasks): Avg Wait: 9.45 ticks

========================================
Results exported to: results/benchmark_20240902_143022.csv
========================================
```

### CSV Sample Output

First few lines of `benchmark_results.csv`:

```
Algorithm,WorkloadType,CPUCount,TaskCount,SimulationDuration,AverageWaitingTime,...
round-robin,cpu-bound,1,1000,45823,125.342,...
round-robin,cpu-bound,1,10000,458234,1253.420,...
round-robin,cpu-bound,2,1000,23912,62.671,...
round-robin,cpu-bound,2,10000,239120,626.710,...
priority,cpu-bound,1,1000,42567,118.234,...
...
```

## Troubleshooting

### Build Fails

```bash
# Check CMake installation
cmake --version

# Try verbose build
cmake --build build --config Release --verbose

# Check compiler
g++ --version  # Linux
clang++ --version  # macOS
cl.exe  # Windows MSVC
```

### Benchmark Crashes

```bash
# Run with error output
./build/bin/benchmark 2>&1 | tee error.log

# Check for incomplete results
tail -5 benchmark_results.csv
```

### CSV File Missing

```bash
# Check build directory
ls -la build/
ls -la build/bin/

# Verify permissions
chmod +x build/bin/benchmark
```

### Reproducibility Failed

```bash
# Ensure seed is consistent (should always be 42)
grep "RandomSeed" results/benchmark_*.csv | uniq -c

# Check system load
top -n 1  # Linux
ps aux   # macOS
```

## Performance Tips

For faster benchmarks:

```bash
# 1. Use Release build
cmake -S .. -B build -DCMAKE_BUILD_TYPE=Release

# 2. Parallel compilation
cmake --build build -- -j$(nproc)  # Linux/macOS
cmake --build build -- /m:8         # Windows

# 3. Don't run other applications
# Close browsers, IDEs, other programs

# 4. Use SSD
# Benchmarks write to disk; SSD significantly faster than HDD
```

## Results Analysis

### Basic Analysis

```bash
# Count results per algorithm
cut -d',' -f1 results/benchmark_*.csv | sort | uniq -c

# Find best throughput
sort -t',' -k10 -nr results/benchmark_*.csv | head -10

# Find worst waiting time
sort -t',' -k6 -nr results/benchmark_*.csv | head -10
```

### Using Python for Analysis

```python
import pandas as pd

# Load results
df = pd.read_csv('results/benchmark_YYYYMMDD_HHMMSS.csv')

# Group by algorithm
for algo in df['Algorithm'].unique():
    subset = df[df['Algorithm'] == algo]
    print(f"{algo}:")
    print(f"  Avg Waiting Time: {subset['AverageWaitingTime'].mean():.2f}")
    print(f"  Avg Throughput: {subset['Throughput'].mean():.2f}")
```

## Next Steps

After running benchmarks:

1. **Review Results**: See [RESULTS_ANALYSIS.md](RESULTS_ANALYSIS.md)
2. **Compare Algorithms**: Analyze which algorithm performs best for your use case
3. **Visualize Data**: Create graphs from CSV data
4. **Document Findings**: Record conclusions and performance insights
