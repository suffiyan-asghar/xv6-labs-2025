# xv6 MLFQ Scheduler Implementation

This directory contains a complete implementation of a Multi-Level Feedback Queue (MLFQ) scheduler for the MIT xv6 operating system targeting RISC-V architecture.

## Overview

The MLFQ scheduler is an adaptive scheduling algorithm that automatically classifies processes based on their behavior:
- **Interactive processes** (I/O-bound): Kept at high priority for low-latency response
- **CPU-bound processes**: Gradually demoted to lower priorities for fairness
- **Mixed behavior**: Dynamically reclassified based on observed CPU consumption

## Key Features

1. **4-Level Priority Queue System**
   - Level 0: Highest priority (4-tick time quantum)
   - Level 1: Medium-high priority (8-tick time quantum)
   - Level 2: Medium-low priority (16-tick time quantum)
   - Level 3: Lowest priority (32-tick time quantum)

2. **Automatic Process Classification**
   - Processes start at highest priority
   - Demoted when they exceed their time quantum
   - Re-promoted periodically to prevent starvation

3. **Starvation Prevention**
   - Priority boost every 256 ticks (≈26ms)
   - All processes reset to highest priority
   - Ensures fair CPU allocation

4. **getprocinfo System Call**
   - Query MLFQ scheduler state for any process
   - Returns: queue level, ticks in queue, total ticks, process state
   - Useful for monitoring and testing

## Building

### Prerequisites
- RISC-V cross-compiler toolchain
- QEMU (version 7.2+) with RISC-V support
- Standard Unix build tools

### Build Commands

```bash
# Clean build
make clean
make

# Run in QEMU
make qemu

# Run with GDB debugging
make qemu-gdb
```

## Testing

### Running MLFQ Demo

```bash
# In QEMU shell
$ mlfq_demo
=== MLFQ Scheduler Demo ===

Test 1: CPU-bound process (should be demoted)
Child PID: 3
Child PID 3: Queue Level = 0, Ticks In Queue = 0, Total Ticks = 100
Child PID 3: Queue Level = 1, Ticks In Queue = 2, Total Ticks = 250
Child PID 3: Queue Level = 2, Ticks In Queue = 0, Total Ticks = 300
Child PID 3: Queue Level = 3, Ticks In Queue = 8, Total Ticks = 500

Test 2: I/O-bound process (should stay high priority)
Child PID: 4
Child PID 4: Queue Level = 0, Ticks In Queue = 0, Total Ticks = 50
Child PID 4: Queue Level = 0, Ticks In Queue = 1, Total Ticks = 100
Child PID 4: Queue Level = 0, Ticks In Queue = 2, Total Ticks = 150
Child PID 4: Queue Level = 0, Ticks In Queue = 3, Total Ticks = 200
```

### Running Comprehensive Test

```bash
$ mlfq_test
# Runs through multiple scenarios testing CPU-bound, I/O-bound, and multiple processes
```

## Architecture

### Data Structures

**Per-Process MLFQ State** (`kernel/proc.h`):
```c
int queue_level;           // Current priority queue (0-3)
uint64 ticks_in_queue;     // Time spent in current queue
uint64 total_ticks;        // Total CPU time consumed
uint64 last_boost_ticks;   // Last time process was boosted
```

**Global MLFQ State** (`kernel/proc.c`):
```c
struct spinlock mlfq_lock;              // Protects MLFQ state
struct proc *mlfq_queues[MLFQ_LEVELS];  // Queue heads
uint64 last_boost_ticks;                // Last global boost time
```

### Key Functions

1. **scheduler()** - Main scheduler loop
   - Scans queues from highest to lowest priority
   - Finds highest-priority RUNNABLE process
   - Performs priority boosts every 256 ticks

2. **yield()** - Enhanced with MLFQ tracking
   - Increments `ticks_in_queue`
   - Checks quantum exceeded
   - Demotes if necessary

3. **mlfq_demote()** - Move process to lower priority
   - Increments queue level (caps at 3)
   - Resets `ticks_in_queue`

4. **mlfq_boost_all()** - Restore all processes to highest priority
   - Called every BOOST_INTERVAL ticks
   - Prevents starvation
   - Handles behavior changes

5. **sys_getprocinfo()** - Query process scheduler state
   - Syscall #22
   - Returns procinfo structure
   - Used for monitoring and testing

## System Call Interface

### getprocinfo()

**Header File**: `user/procinfo.h`

```c
int getprocinfo(int pid, struct procinfo *info);
```

**Parameters**:
- `pid`: Process ID to query
- `info`: Pointer to procinfo structure (output)

**Return Value**:
- 0 on success
- -1 if process not found or error

**procinfo Structure**:
```c
struct procinfo {
  int pid;                 // Process ID
  int queue_level;         // Current priority (0-3)
  uint64 ticks_in_queue;   // Ticks in current queue
  uint64 total_ticks;      // Total ticks consumed
};
```

## Configuration

All MLFQ parameters are configured in `kernel/param.h`:

```c
#define MLFQ_LEVELS       4    // Number of priority levels
#define BOOST_INTERVAL    256  // Boost every 256 ticks
#define TIME_QUANTA_0     4    // Queue 0 time slice
#define TIME_QUANTA_1     8    // Queue 1 time slice
#define TIME_QUANTA_2     16   // Queue 2 time slice
#define TIME_QUANTA_3     32   // Queue 3 time slice
```

### Tuning Guidelines

- **BOOST_INTERVAL**: Increase for more differentiation, decrease for better fairness
- **TIME_QUANTA_X**: Adjust for responsiveness vs. context switch overhead

## Implementation Files

### Kernel Files
- `kernel/proc.h` - Added MLFQ fields to struct proc
- `kernel/proc.c` - MLFQ scheduler implementation and helpers
- `kernel/param.h` - Configuration constants
- `kernel/procinfo.h` - procinfo structure (kernel version)
- `kernel/syscall.h` - Added SYS_getprocinfo constant
- `kernel/syscall.c` - Syscall table entry
- `kernel/sysproc.c` - sys_getprocinfo() implementation

### User Files
- `user/user.h` - getprocinfo() declaration
- `user/procinfo.h` - procinfo structure (user version)
- `user/usys.pl` - Syscall stub generator (added getprocinfo)
- `user/mlfq_demo.c` - Simple demo program
- `user/mlfq_test.c` - Comprehensive test suite

### Documentation
- `MLFQ_DESIGN.md` - Detailed design document
- `MLFQ_README.md` - This file

## Expected Behavior

### CPU-Bound Process
- Starts at queue level 0
- Uses entire time quantum
- Demoted to level 1
- Eventually reaches level 3 after using ~60 ticks total
- Remains at level 3 until priority boost

### I/O-Bound Process
- Starts at queue level 0
- Yields before time quantum expires
- Remains at level 0
- Provides low-latency response

### Priority Boost Example
- Boost interval: 256 ticks
- After 256 ticks, all processes reset to queue level 0
- Prevents indefinite starvation
- Allows CPU-bound processes that become I/O-bound to be reclassified

## Performance

### Advantages
1. Better responsiveness for interactive processes
2. Fair scheduling for CPU-bound workloads
3. No process starvation
4. Automatic process behavior classification
5. Reduced context-switch overhead for CPU-bound work

### Trade-offs
1. More complex than round-robin
2. Requires tuning for specific workloads
3. Slightly higher scheduler overhead
4. Per-process state tracking

## Debugging

### Enable MLFQ Tracing

Add debug output to track queue movements:

```bash
# Check process state in QEMU
xv6$ ps  # Shows all processes

# Monitor specific process
$ mlfq_demo
```

### Common Issues

1. **Process not found in getprocinfo**
   - Check if process has exited (state = ZOMBIE)
   - Verify PID is correct

2. **Unexpected queue levels**
   - Check TIME_QUANTA constants
   - Verify BOOST_INTERVAL isn't too short

3. **Starvation visible**
   - Decrease BOOST_INTERVAL
   - Verify boost code is executing

## References

- "Operating Systems: Three Easy Pieces" (OSTEP) by Remzi Arpaci-Dusseau
- MIT 6.1810 (xv6) Course Materials
- RISC-V ISA Specification

## Future Enhancements

1. **Dynamic Time Quanta** - Adjust based on system load
2. **Priority Decay** - Gradual degradation instead of fixed levels
3. **Per-CPU Queues** - Reduce lock contention
4. **Aging** - More sophisticated starvation prevention
5. **Interactive Classification** - Better I/O vs. CPU detection

## License

Same as xv6 (MIT License)

## Authors

MLFQ Implementation for xv6 - Educational Implementation
Based on OSTEP MLFQ design principles
