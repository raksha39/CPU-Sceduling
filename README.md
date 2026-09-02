# Multi-Core CPU Scheduler and User-Space Thread Runtime

An incremental C++20 scheduler simulator for Windows. It models scheduling decisions deterministically; it is not a kernel scheduler and does not control the Windows scheduler.

## Phase 1 status

The project currently provides the Process model, simulated CPU abstraction, deterministic simulation clock, and SimulationEngine skeleton. See [the architecture document](docs/ARCHITECTURE.md).

## Build

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

GoogleTest is discovered through an installed CMake package. Configure with `-DGTest_DIR=<path>` when necessary.
