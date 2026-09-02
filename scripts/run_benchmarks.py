#!/usr/bin/env python3
"""
Reproducible Benchmark Runner for Multi-Core CPU Scheduler
Builds the project and runs the complete benchmark suite with timestamped results.
"""

import os
import sys
import subprocess
import json
import datetime
import csv
from pathlib import Path

class BenchmarkRunner:
    def __init__(self):
        self.project_root = Path(__file__).parent
        self.build_dir = self.project_root / "build"
        self.results_dir = self.project_root / "results"
        self.benchmark_exe = self.build_dir / "benchmark"
        self.timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        
    def setup_directories(self):
        """Create necessary directories."""
        self.build_dir.mkdir(exist_ok=True)
        self.results_dir.mkdir(exist_ok=True)
        print(f"[SETUP] Directories ready: {self.build_dir}, {self.results_dir}")
        
    def build_project(self):
        """Build the benchmark executable."""
        print("\n" + "="*60)
        print("STEP 1: Building Benchmark Executable")
        print("="*60)
        
        # Check if CMakeLists.txt exists
        cmake_file = self.project_root / "CMakeLists.txt"
        if not cmake_file.exists():
            print(f"ERROR: CMakeLists.txt not found at {cmake_file}")
            sys.exit(1)
            
        try:
            # Configure build
            print(f"Configuring build in {self.build_dir}...")
            subprocess.run(
                ["cmake", "-S", str(self.project_root), "-B", str(self.build_dir)],
                check=True,
                capture_output=False
            )
            
            # Build project
            print("Building benchmark executable...")
            subprocess.run(
                ["cmake", "--build", str(self.build_dir), "--config", "Release", "--", "-j4"],
                check=True,
                capture_output=False
            )
            
            # Verify executable exists
            if not self.benchmark_exe.exists():
                print(f"ERROR: Benchmark executable not created at {self.benchmark_exe}")
                sys.exit(1)
                
            print(f"[SUCCESS] Benchmark executable built: {self.benchmark_exe}")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Build failed: {e}")
            sys.exit(1)
            
    def run_benchmarks(self):
        """Run the benchmark suite."""
        print("\n" + "="*60)
        print("STEP 2: Running Benchmark Suite")
        print("="*60)
        print(f"Timestamp: {self.timestamp}")
        print(f"Working directory: {self.build_dir}")
        
        try:
            # Change to build directory
            os.chdir(self.build_dir)
            
            # Run benchmarks
            print("Starting 300 experiments (3 algorithms × 5 workloads × 5 CPUs × 4 tasks)...")
            print("This may take 5-15 minutes depending on hardware.\n")
            
            result = subprocess.run(
                [str(self.benchmark_exe)],
                check=True,
                capture_output=False
            )
            
            print("\n[SUCCESS] Benchmark suite completed")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"[ERROR] Benchmark execution failed: {e}")
            sys.exit(1)
            
    def process_results(self):
        """Process and archive benchmark results."""
        print("\n" + "="*60)
        print("STEP 3: Processing Results")
        print("="*60)
        
        csv_file = self.build_dir / "benchmark_results.csv"
        
        if not csv_file.exists():
            print(f"ERROR: Results file not found: {csv_file}")
            sys.exit(1)
            
        # Read and validate CSV
        results_count = 0
        try:
            with open(csv_file, 'r') as f:
                reader = csv.DictReader(f)
                rows = list(reader)
                results_count = len(rows)
                
            print(f"[SUCCESS] Read {results_count} experiment results")
            
            # Validate structure
            if results_count != 300:
                print(f"[WARNING] Expected 300 experiments, got {results_count}")
            else:
                print("[SUCCESS] Correct number of experiments (300)")
                
        except Exception as e:
            print(f"ERROR: Failed to process CSV: {e}")
            sys.exit(1)
            
        # Archive results with timestamp
        archived_file = self.results_dir / f"benchmark_{self.timestamp}.csv"
        try:
            with open(csv_file, 'r') as src:
                with open(archived_file, 'w') as dst:
                    dst.write(src.read())
            print(f"[SUCCESS] Archived results to: {archived_file}")
        except Exception as e:
            print(f"[ERROR] Failed to archive results: {e}")
            sys.exit(1)
            
        # Generate summary report
        self.generate_summary_report(rows, archived_file)
        
    def generate_summary_report(self, rows, results_file):
        """Generate summary statistics from results."""
        print("\n" + "="*60)
        print("STEP 4: Summary Report")
        print("="*60)
        
        # Group by algorithm
        by_algo = {}
        for row in rows:
            algo = row.get('Algorithm', 'unknown')
            if algo not in by_algo:
                by_algo[algo] = []
            by_algo[algo].append(row)
            
        print("\nAlgorithm Summary:")
        for algo, results in by_algo.items():
            avg_wait = sum(float(r.get('AverageWaitingTime', 0)) for r in results) / len(results)
            avg_turnaround = sum(float(r.get('AverageTurnaroundTime', 0)) for r in results) / len(results)
            avg_throughput = sum(float(r.get('Throughput', 0)) for r in results) / len(results)
            
            print(f"\n  {algo.upper()}:")
            print(f"    Experiments: {len(results)}")
            print(f"    Avg Waiting Time: {avg_wait:.2f} ticks")
            print(f"    Avg Turnaround Time: {avg_turnaround:.2f} ticks")
            print(f"    Avg Throughput: {avg_throughput:.2f} proc/1000 ticks")
            
        # Workload summary
        print("\nWorkload Summary:")
        by_workload = {}
        for row in rows:
            workload = row.get('WorkloadType', 'unknown')
            if workload not in by_workload:
                by_workload[workload] = []
            by_workload[workload].append(row)
            
        for workload, results in sorted(by_workload.items()):
            avg_wait = sum(float(r.get('AverageWaitingTime', 0)) for r in results) / len(results)
            print(f"  {workload}: Avg Wait {avg_wait:.2f} ticks ({len(results)} experiments)")
            
    def create_metadata(self):
        """Create benchmark metadata file."""
        metadata = {
            "timestamp": self.timestamp,
            "benchmark_framework": "Multi-Core CPU Scheduler",
            "experiments": 300,
            "algorithms": ["round-robin", "priority", "mlfq"],
            "workloads": ["cpu-bound", "io-bound", "mixed", "bursty", "random"],
            "cpu_counts": [1, 2, 4, 8, 16],
            "task_counts": [1000, 10000, 50000, 100000],
            "random_seed": 42,
            "reproducible": True,
            "hostname": os.environ.get('COMPUTERNAME', 'unknown'),
            "python_version": sys.version
        }
        
        metadata_file = self.results_dir / f"metadata_{self.timestamp}.json"
        try:
            with open(metadata_file, 'w') as f:
                json.dump(metadata, f, indent=2)
            print(f"\n[SUCCESS] Metadata saved to: {metadata_file}")
        except Exception as e:
            print(f"[WARNING] Failed to save metadata: {e}")
            
    def run(self):
        """Execute the complete benchmark workflow."""
        print("\n" + "="*70)
        print("MULTI-CORE CPU SCHEDULER - REPRODUCIBLE BENCHMARK SUITE")
        print("="*70)
        print(f"Project Root: {self.project_root}")
        print(f"Build Directory: {self.build_dir}")
        print(f"Results Directory: {self.results_dir}")
        
        try:
            self.setup_directories()
            self.build_project()
            self.run_benchmarks()
            self.process_results()
            self.create_metadata()
            
            print("\n" + "="*70)
            print("BENCHMARK SUITE COMPLETED SUCCESSFULLY")
            print("="*70)
            print(f"Results file: {self.results_dir / f'benchmark_{self.timestamp}.csv'}")
            print("Next steps:")
            print("  1. Open the CSV file in a spreadsheet application")
            print("  2. Generate charts to analyze algorithm performance")
            print("  3. See BENCHMARKING.md for interpretation guide")
            print("="*70 + "\n")
            
            return 0
            
        except KeyboardInterrupt:
            print("\n\n[CANCELLED] Benchmark run interrupted by user")
            return 1
        except Exception as e:
            print(f"\n\n[FATAL ERROR] {e}")
            import traceback
            traceback.print_exc()
            return 1


if __name__ == "__main__":
    runner = BenchmarkRunner()
    sys.exit(runner.run())
