# Architecture

## Scope and boundary

This project is a deterministic, user-space scheduler simulator. Simulated CPUs and processes are domain objects; they do not control physical CPU execution. A future optional runtime will run worker `std::thread`s, which remain scheduled by Windows.

## Component diagram

```text
CLI/configuration -> Workload generator -> SimulationEngine -> per-CPU run queues -> Scheduler policy
                                              |                   |                    |
                                              v                   v                    v
                                         Process lifecycle     CPU model         Events + Metrics -> CSV
                                                                    ^
Load balancer / work stealing / affinity / migration -------------|
```

## Implemented components (Phases 1–2)

| Component | Responsibility |
| --- | --- |
| `Process` | Owns immutable identity/workload inputs and mutable lifecycle/execution metadata. It validates state transitions and affinity. |
| `Cpu` | Represents one simulated core, owns its current process reference, and records busy ticks. |
| `ReadyQueue` | FIFO queue of READY processes. It preserves insertion order while selecting the first task eligible for a requesting CPU's affinity constraints. |
| `Scheduler` | Policy-only abstraction for enqueueing, selection, tick accounting, preemption decisions, completion, and wakeup handling. It has no dependency on `Cpu`. |
| `FcfsScheduler` | Non-preemptive FIFO policy. |
| `RoundRobinScheduler` | FIFO policy with a validated, configurable positive time quantum and per-simulated-CPU slice accounting. |
| `SimulationEngine` | Owns simulated time, CPUs, submitted processes, the selected scheduler, and an ordered event history. It admits arrivals, delegates dispatch selection, executes one tick, and invokes policy lifecycle hooks. |

The Phase 1 first-ready placeholder has been replaced with the scheduler interface. Per-core queues are deferred to the multi-core phase; Phase 2 has one policy-owned ready queue to establish the policy contract.

## Process lifecycle

```text
NEW -> READY -> RUNNING -> TERMINATED
                  ^   |       |
                  |   +-------+ preemption
                  |           |
WAITING ----------+ <---------+ blocking (future I/O model)
```

Invalid transitions throw `std::logic_error`. A process is dispatched only when READY, can execute only while RUNNING, and completes only after remaining work reaches zero.

## Simulation flow

For every deterministic tick `t`, the engine:

1. Moves arrived `NEW` processes to `READY`.
2. Adds arrived tasks to the configured scheduler and dispatches the first affinity-eligible selection to each idle CPU.
3. Executes every running process for one unit of simulated CPU time.
4. Marks exhausted processes TERMINATED at `t + 1`; otherwise it notifies the scheduler and applies any requested preemption.
5. Records arrival, dispatch, preemption, and completion events, then increments the simulation clock.

Future phases add wakeups, additional policies, context-switch costs, per-core queues, balancing, and work stealing.

## Multi-core and affinity flow

`SimulationEngine` constructs the requested number of simulated `Cpu` objects, independent of the host machine. A process with an empty affinity vector may run anywhere. Otherwise, dispatch validates that the CPU identifier belongs to its sorted affinity set. The future placement/migration layer will retain that invariant.

## Intended scheduling flow

Schedulers use a policy-only interface: enqueue/add process, select next, tick notification, preemption decision, completion, preemption, and wakeup handling. CPUs remain policy-agnostic. FCFS and Round Robin are implemented; Priority and MLFQ follow in later phases.

## Load balancing, stealing, and migration (planned)

Each CPU will gain a local run queue. Periodic balancing will compare documented queue/work estimates, choose affinity-eligible READY tasks, and migrate them with a recorded overhead. Idle CPU stealing will be independent of periodic balancing. Every migration will update metadata and emit an event.

## Synchronization strategy (planned runtime)

The core simulator stays single-threaded and deterministic. The optional user-space runtime will use worker threads, a mutex per independent queue where practical, condition variables for sleeping workers, and atomics for lifecycle counters. The runtime will not be conflated with simulator semantics.

## Benchmark pipeline (planned)

```text
seeded workload -> configured simulation -> event/metric aggregation -> CSV -> pandas/matplotlib plots
```

Fixed seeds and deterministic clocks make repeated experiments comparable. Benchmark numbers will only be reported from generated CSV output.
