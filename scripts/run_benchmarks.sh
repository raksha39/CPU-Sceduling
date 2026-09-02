#!/bin/bash
# Reproducible Benchmark Runner for Multi-Core CPU Scheduler (Bash)
# Usage: ./scripts/run_benchmarks.sh

set -e  # Exit on error

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'  # No Color

# Functions for colored output
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_info() { echo -e "${CYAN}[INFO]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
RESULTS_DIR="$PROJECT_ROOT/results"
BENCHMARK_EXE="$BUILD_DIR/benchmark"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SKIP_BUILD=false
CLEAN=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-build) SKIP_BUILD=true; shift ;;
        --clean) CLEAN=true; shift ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --skip-build    Skip compilation step"
            echo "  --clean         Clean build directory before building"
            echo "  --help          Show this help message"
            exit 0
            ;;
        *) print_error "Unknown option: $1"; exit 1 ;;
    esac
done

# Print header
echo ""
echo "================================================================================"
echo "MULTI-CORE CPU SCHEDULER - REPRODUCIBLE BENCHMARK SUITE"
echo "================================================================================"
print_info "Project Root: $PROJECT_ROOT"
print_info "Build Directory: $BUILD_DIR"
print_info "Results Directory: $RESULTS_DIR"
print_info "Timestamp: $TIMESTAMP"
echo ""

# Validate project structure
validate_project() {
    if [[ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]]; then
        print_error "CMakeLists.txt not found at $PROJECT_ROOT/CMakeLists.txt"
        exit 1
    fi
    print_success "Project structure validated"
}

# Setup directories
setup_directories() {
    print_info "Creating directories..."
    mkdir -p "$BUILD_DIR"
    mkdir -p "$RESULTS_DIR"
    print_success "Directories ready"
}

# Clean build (optional)
clean_build() {
    if [[ "$CLEAN" == true ]]; then
        print_warning "Cleaning previous build..."
        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

# Build project
build_project() {
    if [[ "$SKIP_BUILD" == true ]]; then
        print_info "Build step skipped"
        return 0
    fi
    
    echo ""
    echo "================================================================================"
    echo "STEP 1: Building Benchmark Executable"
    echo "================================================================================"
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Please install CMake"
        exit 1
    fi
    
    # Check for compiler
    if ! command -v c++ &> /dev/null; then
        print_error "C++ compiler not found. Please install a C++ compiler"
        exit 1
    fi
    
    # Configure build
    print_info "Configuring build in $BUILD_DIR..."
    if ! cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release; then
        print_error "CMake configuration failed"
        exit 1
    fi
    
    # Build project
    print_info "Building benchmark executable..."
    if ! cmake --build "$BUILD_DIR" --config Release -- -j4; then
        print_error "Build failed"
        exit 1
    fi
    
    # Verify executable
    if [[ ! -f "$BENCHMARK_EXE" ]]; then
        print_error "Benchmark executable not created at $BENCHMARK_EXE"
        exit 1
    fi
    
    print_success "Benchmark executable built successfully"
    print_success "Executable: $BENCHMARK_EXE"
}

# Run benchmarks
run_benchmarks() {
    echo ""
    echo "================================================================================"
    echo "STEP 2: Running Benchmark Suite"
    echo "================================================================================"
    
    if [[ ! -f "$BENCHMARK_EXE" ]]; then
        print_error "Benchmark executable not found at $BENCHMARK_EXE"
        print_error "Please build the project first (remove --skip-build flag)"
        exit 1
    fi
    
    print_info "Starting 300 experiments:"
    print_info "  3 algorithms × 5 workloads × 5 CPU counts × 4 task counts"
    print_warning "This may take 5-15 minutes depending on hardware..."
    echo ""
    
    START_TIME=$(date +%s)
    
    if ! (cd "$BUILD_DIR" && "$BENCHMARK_EXE"); then
        print_error "Benchmark execution failed"
        exit 1
    fi
    
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))
    
    print_success "Benchmarks completed in $ELAPSED seconds"
}

# Process results
process_results() {
    echo ""
    echo "================================================================================"
    echo "STEP 3: Processing Results"
    echo "================================================================================"
    
    CSV_FILE="$BUILD_DIR/benchmark_results.csv"
    
    if [[ ! -f "$CSV_FILE" ]]; then
        print_error "Results file not found at $CSV_FILE"
        exit 1
    fi
    
    # Count results
    RESULT_COUNT=$(grep -c . "$CSV_FILE")
    RESULT_COUNT=$((RESULT_COUNT - 1))  # Subtract header line
    
    print_success "Read $RESULT_COUNT experiment results"
    
    if [[ "$RESULT_COUNT" -eq 300 ]]; then
        print_success "Correct number of experiments (300)"
    else
        print_warning "Expected 300 experiments, got $RESULT_COUNT"
    fi
    
    # Archive results
    ARCHIVED_FILE="$RESULTS_DIR/benchmark_$TIMESTAMP.csv"
    if cp "$CSV_FILE" "$ARCHIVED_FILE"; then
        print_success "Results archived to: $ARCHIVED_FILE"
    else
        print_error "Failed to archive results"
        exit 1
    fi
    
    # Generate summary
    generate_summary "$ARCHIVED_FILE"
}

# Generate summary report
generate_summary() {
    local csv_file="$1"
    
    echo ""
    echo "================================================================================"
    echo "STEP 4: Summary Report"
    echo "================================================================================"
    
    # Algorithm summary
    print_info ""
    print_info "Algorithm Summary:"
    
    for algo in "round-robin" "priority" "mlfq"; do
        COUNT=$(grep "^$algo," "$csv_file" | wc -l)
        if [[ $COUNT -gt 0 ]]; then
            AVG_WAIT=$(grep "^$algo," "$csv_file" | awk -F',' '{sum+=$6; count++} END {printf "%.2f", sum/count}')
            AVG_TURNAROUND=$(grep "^$algo," "$csv_file" | awk -F',' '{sum+=$7; count++} END {printf "%.2f", sum/count}')
            AVG_THROUGHPUT=$(grep "^$algo," "$csv_file" | awk -F',' '{sum+=$10; count++} END {printf "%.2f", sum/count}')
            
            echo "  ${algo^^}:"
            echo "    Experiments: $COUNT"
            echo "    Avg Waiting Time: $AVG_WAIT ticks"
            echo "    Avg Turnaround Time: $AVG_TURNAROUND ticks"
            echo "    Avg Throughput: $AVG_THROUGHPUT proc/1000 ticks"
        fi
    done
    
    # Workload summary
    print_info ""
    print_info "Workload Summary:"
    
    for workload in "cpu-bound" "io-bound" "mixed" "bursty" "random"; do
        COUNT=$(grep ",$workload," "$csv_file" | wc -l)
        if [[ $COUNT -gt 0 ]]; then
            AVG_WAIT=$(grep ",$workload," "$csv_file" | awk -F',' '{sum+=$6; count++} END {printf "%.2f", sum/count}')
            echo "  $workload: Avg Wait $AVG_WAIT ticks ($COUNT experiments)"
        fi
    done
}

# Create metadata
create_metadata() {
    local metadata_file="$RESULTS_DIR/metadata_$TIMESTAMP.txt"
    
    cat > "$metadata_file" << EOF
Benchmark Metadata
==================
Timestamp: $TIMESTAMP
Framework: Multi-Core CPU Scheduler
Total Experiments: 300
Algorithms: 3 (round-robin, priority, mlfq)
Workloads: 5 (cpu-bound, io-bound, mixed, bursty, random)
CPU Counts: 5 (1, 2, 4, 8, 16)
Task Counts: 4 (1000, 10000, 50000, 100000)
Random Seed: 42 (deterministic)
Reproducible: Yes

Hostname: $(hostname)
Kernel: $(uname -s)
Architecture: $(uname -m)
Shell: $SHELL
EOF
    
    print_success "Metadata saved to: $metadata_file"
}

# Main execution
main() {
    validate_project
    setup_directories
    clean_build
    build_project
    run_benchmarks
    process_results
    create_metadata
    
    echo ""
    echo "================================================================================"
    print_success "BENCHMARK SUITE COMPLETED SUCCESSFULLY"
    echo "================================================================================"
    print_info "Results file: $RESULTS_DIR/benchmark_$TIMESTAMP.csv"
    print_info "Next steps:"
    print_info "  1. Open the CSV file in a spreadsheet application"
    print_info "  2. Generate charts to analyze algorithm performance"
    print_info "  3. See BENCHMARKING.md for interpretation guide"
    echo "================================================================================"
    echo ""
}

# Run main
main
