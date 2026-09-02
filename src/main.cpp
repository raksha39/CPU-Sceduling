#include "simulation.h"

#include <iostream>

int main() {
    scheduler::SimulationEngine simulation{1};
    std::cout << "Multi-Core CPU Scheduler simulator (Phase 2)\n"
              << "Simulation clock: " << simulation.now() << '\n';
    return 0;
}
