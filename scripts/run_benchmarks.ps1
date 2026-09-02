# Reproducible Benchmark Runner for Multi-Core CPU Scheduler (PowerShell)
# Usage: .\run_benchmarks.ps1

param(
    [string]$ProjectRoot = $PSScriptRoot,
    [switch]$SkipBuild = $false,
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Error { Write-Host $args -ForegroundColor Red }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }

# Paths
$buildDir = Join-Path $ProjectRoot "build"
$resultsDir = Join-Path $ProjectRoot "results"
$benchmarkExe = Join-Path $buildDir "benchmark.exe"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

Write-Host "================================================================================"
Write-Host "MULTI-CORE CPU SCHEDULER - REPRODUCIBLE BENCHMARK SUITE"
Write-Host "================================================================================"
Write-Info "Project Root: $ProjectRoot"
Write-Info "Build Directory: $buildDir"
Write-Info "Results Directory: $resultsDir"
Write-Info "Timestamp: $timestamp"
Write-Host ""

# Validate project structure
function Validate-Project {
    $cmakefile = Join-Path $ProjectRoot "CMakeLists.txt"
    if (-not (Test-Path $cmakefile)) {
        Write-Error "ERROR: CMakeLists.txt not found at $cmakefile"
        exit 1
    }
    Write-Success "[SETUP] Project structure validated"
}

# Setup directories
function Setup-Directories {
    Write-Info "Creating directories..."
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
    New-Item -ItemType Directory -Path $resultsDir -Force | Out-Null
    Write-Success "[SETUP] Directories ready"
}

# Clean build (optional)
function Clean-Build {
    if ($Clean) {
        Write-Warning "Cleaning previous build..."
        Remove-Item -Path $buildDir -Recurse -Force -ErrorAction SilentlyContinue
        New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
        Write-Success "[CLEAN] Build directory cleaned"
    }
}

# Build project
function Build-Project {
    if ($SkipBuild) {
        Write-Info "[SKIP] Build step skipped"
        return
    }
    
    Write-Host ""
    Write-Host "================================================================================"
    Write-Host "STEP 1: Building Benchmark Executable"
    Write-Host "================================================================================"
    
    try {
        # Check for CMake
        $cmake = Get-Command cmake -ErrorAction SilentlyContinue
        if (-not $cmake) {
            Write-Error "ERROR: CMake not found. Please install CMake and add it to PATH"
            exit 1
        }
        
        # Configure build
        Write-Info "Configuring build in $buildDir..."
        & cmake -S $ProjectRoot -B $buildDir
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] CMake configuration failed"
            exit 1
        }
        
        # Build project
        Write-Info "Building benchmark executable..."
        & cmake --build $buildDir --config Release -- /m:4
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Build failed"
            exit 1
        }
        
        # Verify executable
        if (-not (Test-Path $benchmarkExe)) {
            Write-Error "ERROR: Benchmark executable not created at $benchmarkExe"
            exit 1
        }
        
        Write-Success "[BUILD] Benchmark executable built successfully"
        Write-Success "Executable: $benchmarkExe"
        
    } catch {
        Write-Error "ERROR: Build failed: $_"
        exit 1
    }
}

# Run benchmarks
function Run-Benchmarks {
    Write-Host ""
    Write-Host "================================================================================"
    Write-Host "STEP 2: Running Benchmark Suite"
    Write-Host "================================================================================"
    
    if (-not (Test-Path $benchmarkExe)) {
        Write-Error "ERROR: Benchmark executable not found at $benchmarkExe"
        Write-Error "Please build the project first (remove -SkipBuild flag)"
        exit 1
    }
    
    Write-Info "Starting 300 experiments:"
    Write-Info "  3 algorithms × 5 workloads × 5 CPU counts × 4 task counts"
    Write-Warning "This may take 5-15 minutes depending on hardware..."
    Write-Host ""
    
    try {
        Push-Location $buildDir
        
        $startTime = Get-Date
        & $benchmarkExe
        $elapsed = (Get-Date) - $startTime
        
        Pop-Location
        
        Write-Success "[BENCHMARK] Completed in $($elapsed.TotalSeconds) seconds"
        
    } catch {
        Write-Error "ERROR: Benchmark execution failed: $_"
        exit 1
    }
}

# Process results
function Process-Results {
    Write-Host ""
    Write-Host "================================================================================"
    Write-Host "STEP 3: Processing Results"
    Write-Host "================================================================================"
    
    $csvFile = Join-Path $buildDir "benchmark_results.csv"
    
    if (-not (Test-Path $csvFile)) {
        Write-Error "ERROR: Results file not found at $csvFile"
        exit 1
    }
    
    # Read and validate CSV
    try {
        $results = @(Import-Csv -Path $csvFile)
        $resultCount = $results.Count
        
        Write-Success "[RESULTS] Read $resultCount experiment results"
        
        if ($resultCount -eq 300) {
            Write-Success "[VALIDATION] Correct number of experiments (300)"
        } else {
            Write-Warning "[VALIDATION] Expected 300 experiments, got $resultCount"
        }
        
    } catch {
        Write-Error "ERROR: Failed to process CSV: $_"
        exit 1
    }
    
    # Archive results
    $archivedFile = Join-Path $resultsDir "benchmark_$timestamp.csv"
    try {
        Copy-Item -Path $csvFile -Destination $archivedFile -Force
        Write-Success "[ARCHIVE] Results saved to: $archivedFile"
    } catch {
        Write-Error "ERROR: Failed to archive results: $_"
        exit 1
    }
    
    # Generate summary
    Generate-Summary $results $archivedFile
}

# Generate summary report
function Generate-Summary {
    param(
        [array]$Results,
        [string]$ResultsFile
    )
    
    Write-Host ""
    Write-Host "================================================================================"
    Write-Host "STEP 4: Summary Report"
    Write-Host "================================================================================"
    
    # Algorithm summary
    Write-Info "`nAlgorithm Summary:"
    $algorithms = $Results.Algorithm | Select-Object -Unique
    
    foreach ($algo in $algorithms) {
        $algoResults = $Results | Where-Object { $_.Algorithm -eq $algo }
        $avgWait = ($algoResults.AverageWaitingTime | Measure-Object -Average).Average
        $avgTurnaround = ($algoResults.AverageTurnaroundTime | Measure-Object -Average).Average
        $avgThroughput = ($algoResults.Throughput | Measure-Object -Average).Average
        
        Write-Host "  $($algo.ToUpper()):"
        Write-Host "    Experiments: $($algoResults.Count)"
        Write-Host "    Avg Waiting Time: $([Math]::Round($avgWait, 2)) ticks"
        Write-Host "    Avg Turnaround Time: $([Math]::Round($avgTurnaround, 2)) ticks"
        Write-Host "    Avg Throughput: $([Math]::Round($avgThroughput, 2)) proc/1000 ticks"
    }
    
    # Workload summary
    Write-Info "`nWorkload Summary:"
    $workloads = $Results.WorkloadType | Select-Object -Unique | Sort-Object
    
    foreach ($workload in $workloads) {
        $wlResults = $Results | Where-Object { $_.WorkloadType -eq $workload }
        $avgWait = ($wlResults.AverageWaitingTime | Measure-Object -Average).Average
        Write-Host "  $workload`: Avg Wait $([Math]::Round($avgWait, 2)) ticks ($($wlResults.Count) experiments)"
    }
    
    # CPU count scaling
    Write-Info "`nCPU Count Scaling (Throughput):"
    $cpuCounts = $Results.CPUCount | Select-Object -Unique | Sort-Object
    
    foreach ($cpuCount in $cpuCounts) {
        $cpuResults = $Results | Where-Object { $_.CPUCount -eq $cpuCount }
        $avgThroughput = ($cpuResults.Throughput | Measure-Object -Average).Average
        Write-Host "  $cpuCount CPUs: $([Math]::Round($avgThroughput, 2)) proc/1000 ticks (avg)"
    }
}

# Create metadata
function Create-Metadata {
    $metadata = @{
        "timestamp" = $timestamp
        "benchmark_framework" = "Multi-Core CPU Scheduler"
        "experiments" = 300
        "algorithms" = @("round-robin", "priority", "mlfq")
        "workloads" = @("cpu-bound", "io-bound", "mixed", "bursty", "random")
        "cpu_counts" = @(1, 2, 4, 8, 16)
        "task_counts" = @(1000, 10000, 50000, 100000)
        "random_seed" = 42
        "reproducible" = $true
        "hostname" = $env:COMPUTERNAME
        "powershell_version" = $PSVersionTable.PSVersion.ToString()
    }
    
    $metadataFile = Join-Path $resultsDir "metadata_$timestamp.json"
    try {
        $metadata | ConvertTo-Json | Out-File -FilePath $metadataFile -Encoding UTF8
        Write-Success "[METADATA] Saved to: $metadataFile"
    } catch {
        Write-Warning "[WARNING] Failed to save metadata: $_"
    }
}

# Main execution
try {
    Validate-Project
    Setup-Directories
    Clean-Build
    Build-Project
    Run-Benchmarks
    Process-Results
    Create-Metadata
    
    Write-Host ""
    Write-Host "================================================================================"
    Write-Success "BENCHMARK SUITE COMPLETED SUCCESSFULLY"
    Write-Host "================================================================================"
    Write-Info "Results file: $resultsDir\benchmark_$timestamp.csv"
    Write-Info "Next steps:"
    Write-Info "  1. Open the CSV file in Excel or another spreadsheet application"
    Write-Info "  2. Generate charts to analyze algorithm performance"
    Write-Info "  3. See BENCHMARKING.md for interpretation guide"
    Write-Host "================================================================================"
    Write-Host ""
    
} catch {
    Write-Error "FATAL ERROR: $_"
    exit 1
}
