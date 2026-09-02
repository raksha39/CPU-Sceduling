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
| `PriorityScheduler` | Stable priority queue policy with configurable priority direction. A strictly higher-priority, affinity-eligible READY task preempts a running task before its next simulated CPU tick. |
| `MlfqScheduler` | Configurable multilevel feedback policy with separate FIFO queues, quantum-based demotion, aging, and periodic priority boosts. |
| `SimulationEngine` | Owns simulated time, CPUs, submitted processes, one scheduler instance per CPU, and an ordered event history. It admits arrivals, performs deterministic initial placement, executes every core for one tick, and invokes the matching local policy hooks. |

Each simulated CPU has an independent scheduler instance and therefore an independent local ready queue. Scheduler state (such as RR time slices and MLFQ queue levels) is not shared across CPUs.

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

The event history includes arrival, dispatch, preemption, logical context-switch, and completion events. Context-switch *cost* is deferred to Phase 5. Future phases add wakeups, additional policies, per-core queues, balancing, and work stealing.

## Multi-core and affinity flow

`SimulationEngine` constructs the requested number of simulated `Cpu` objects, independent of the host machine. A process with an empty affinity vector may run anywhere. Otherwise, dispatch validates that the CPU identifier belongs to its sorted affinity set. At arrival, initial placement selects the affinity-eligible CPU with the least local runnable load (READY count plus a running task); ties use the lowest CPU ID. The process is then owned by that CPU's local queue. No migration, balancing, or stealing occurs in this phase.

Each `Cpu` tracks its current process, busy ticks, and local logical context-switch count. Since a process is inserted into exactly one local queue and held by at most one `Cpu`, it cannot execute on more than one simulated CPU at a time.

## Intended scheduling flow

Schedulers use a policy-only interface: enqueue/add process, select next, tick notification, preemption decision, completion, preemption, and wakeup handling. CPUs remain policy-agnostic. FCFS, Round Robin, preemptive Priority, and MLFQ are implemented.

## MLFQ design

The default MLFQ has three FIFO queues: Q0 uses quantum 2, Q1 uses quantum 4, and Q2 is FCFS. New tasks enter Q0. When a task consumes an entire finite quantum, it is requeued one level lower; Q2 cannot be demoted further. A ready task in a higher queue preempts a lower-queue running task.

An interactive task that blocks or voluntarily yields before its quantum expires retains its current queue on wakeup. This rewards short/interactive bursts without promoting a task merely for blocking. (The simulated I/O blocking source is introduced in a later phase; the scheduler hook and policy are already defined.)

`agingThreshold` promotes a READY task by one queue after it has waited that many simulated ticks; zero disables aging. `boostInterval` moves every READY task from lower queues to Q0 at each positive interval; zero disables boosts. Both mechanisms prevent indefinite starvation. Queue transitions are emitted as `QUEUE_CHANGE` events with a reason. FIFO insertion order makes equal-level selection deterministic.

## Load balancing, stealing, and migration (planned)

Each CPU will gain a local run queue. Periodic balancing will compare documented queue/work estimates, choose affinity-eligible READY tasks, and migrate them with a recorded overhead. Idle CPU stealing will be independent of periodic balancing. Every migration will update metadata and emit an event.

## Synchronization strategy (planned runtime)

The core simulator stays single-threaded and deterministic. The optional user-space runtime will use worker threads, a mutex per independent queue where practical, condition variables for sleeping workers, and atomics for lifecycle counters. The runtime will not be conflated with simulator semantics.

## Benchmark pipeline (planned)

```text
seeded workload -> configured simulation -> event/metric aggregation -> CSV -> pandas/matplotlib plots
```

Fixed seeds and deterministic clocks make repeated experiments comparable. Benchmark numbers will only be reported from generated CSV output.
