# FlowLock — Concurrent Task Orchestrator Without Mutexes

FlowLock is now public after being a private project I developed to solve a very specific internal need: getting rid of the logistical nightmare caused by `mutex` usage. They were too rigid, too verbose, and a real pain to maintain in a fast-paced and parallel development environment.

## Purpose

Provide a lightweight, robust, and high-performance **C++17** library for concurrent task execution with:
- Fine-grained priority management (32-bit integers, default descending sort)
- Conflict detection system based on tags and policies (EXCLUSIVE, SHARED, PRIORITY)
- Built-in profiling mechanism via `FlowContext`, easily toggleable
- Configurable `FlowScheduler` supporting FIFO and PRIORITY strategies
- Seamless integration with CI/CD environments (thanks to clear API design and structured logs)
- Stable, predictable, and reproducible behavior in production

## Core Components

![image](https://github.com/user-attachments/assets/822de100-4c8a-4739-afa4-9e6ec1c4a13f)

### `FlowLock`
Central singleton that coordinates the entire system. I use it to:
- Submit tasks via `request(...)`
- Execute the task queue with `run()`
- Wait for all tasks to complete with `await()`

### `FlowTask`
Encapsulates a user-defined function with:
- An integer priority (default = 0)
- One or more tags for conflict resolution
- A timestamp to preserve FIFO order at equal priority

### `ConflictResolver`
Applies conflict resolution policies per tag:
- `EXCLUSIVE`: only one task per tag runs at a time
- `SHARED`: multiple concurrent tasks allowed
- `PRIORITY`: higher priority tasks override lower ones

### `FlowScheduler`
Queues and selects tasks to run. Uses `PRIORITY` by default, `FIFO` also available.

### `FlowExecution`
Executes tasks, handles errors, captures exceptions, and invokes user-defined callbacks on task completion.

### `FlowContext`
Provides:
- A thread ID
- A unique logical tick
- Profiling capabilities for measuring execution duration

### `FlowTracer`
Full tracing of scheduler activity for debugging and auditing. Records:
- Scheduler queue empty events
- Detected conflicts
- Execution errors
- Task durations

### `FlowSection`
RAII-style structure for grouping code blocks into a named and traceable section.

## Usage Example

```cpp
FlowSection section("AudioProcessing", 7, {"audio"});
section << [](FlowContext& ctx) {
    // Intensive audio processing
};
FlowLock::instance().run();
FlowLock::instance().await();
```

## CI/CD and Production Readiness

FlowLock is designed to operate seamlessly in automated build pipelines and production-grade multithreaded backends. It provides:
- A clean, unit-testable API
- Structured logs for analysis and monitoring
- Non-blocking execution in case of conflicts, with automatic rescheduling

## Why FlowLock?

I created this library to replace `mutex`, which caused recurring issues in my project:
- Subtle deadlocks
- Hard-to-trace performance bottlenecks
- Inflexibility in orchestrating dynamic asynchronous logic

FlowLock allowed me to centralize all concurrent execution logic into a scalable, traceable, and stable system.

## Technical Specifications

- Language: **C++17** (uses `std::shared_ptr`, `std::optional`, `std::function`, `std::atomic`, `std::chrono`, etc.)
- Controlled dynamic allocation (tasks are wrapped in `shared_ptr`)
- Thread safety via localized internal mutexes
- Compatible with Linux, macOS, Windows (compilers: GCC, Clang, MSVC)
- No external library dependencies

## License and Support

This project is released as open source to be useful to others. Contributions are welcome, but active support is not guaranteed. The code is tested and intended to work out-of-the-box.

No more mutexes. That alone is a big win.
