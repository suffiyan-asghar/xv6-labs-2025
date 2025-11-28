# MLFQ Scheduler Implementation for xv6
## Design Document

### 1. Overview
This document describes the implementation of a Multi-Level Feedback Queue (MLFQ) scheduler in the MIT xv6 operating system for RISC-V. The MLFQ scheduler improves upon xv6's default round-robin scheduler by providing adaptive priority management based on process behavior.

### 2. MLFQ Design Principles

#### 2.1 Queue Structure
- **4 Priority Levels**: The system maintains 4 distinct priority queues (levels 0-3)
  - Level 0: Highest priority (interactive/I/O-bound processes)
  - Level 3: Lowest priority (CPU-bound processes)
- **Round-Robin Within Levels**: Processes at the same priority level share CPU time using round-robin scheduling
- **Strict Priority**: Higher priority queues are scheduled before lower priority queues

#### 2.2 Time Quanta (Time Slices)
Different queue levels have different time quanta to balance responsiveness and efficiency:
- Queue 0: 4 ticks (highest responsiveness for interactive processes)
- Queue 1: 8 ticks
- Queue 2: 16 ticks
- Queue 3: 32 ticks (longer slices for CPU-bound work)

#### 2.3 MLFQ Rules

**Rule 1: Priority-Based Scheduling**
- If Priority(A) > Priority(B), then A runs (B waits)

**Rule 2: Round-Robin at Same Priority**
- If Priority(A) = Priority(B), both A and B run using round-robin scheduling

**Rule 3: Initial Placement**
- When a job enters the system, it is placed at the highest priority (queue level 0)
- This allows quick classification of process behavior

**Rule 4: Demotion on Time Quantum Exceeded**
- Once a process consumes its time allotment at a given level (ticks_in_queue >= time_quantum), it is demoted one level lower
- This identifies CPU-bound processes and moves them to lower priority queues
- Anti-gaming: The scheduler tracks total time at each level regardless of yields

**Rule 5: Starvation Prevention via Priority Boost**
- Every BOOST_INTERVAL ticks (256 ticks), all processes are boosted back to the top queue
- This ensures low-priority processes eventually get CPU time
- Allows CPU-bound processes that become interactive to be re-classified

### 3. Implementation Details

#### 3.1 Data Structures

**Process Control Block (struct proc) Extensions:**
```c
int queue_level;           // Current priority queue (0=highest, 3=lowest)
uint64 ticks_in_queue;     // Time spent in current queue (in ticks)
uint64 total_ticks;        // Total ticks consumed by process
uint64 last_boost_ticks;   // Last time this process was boosted
```

**Global MLFQ State:**
- `mlfq_queues[MLFQ_LEVELS]`: Array of queue heads (for potential queue-based implementation)
- `mlfq_lock`: Spinlock protecting MLFQ state
- `last_boost_ticks`: Global variable tracking last priority boost time

#### 3.2 Key Functions

**Scheduler (scheduler function):**
- Scans queues from highest to lowest priority
- Finds first RUNNABLE process at highest priority level
- Performs priority boost check every scheduler iteration
- Maintains CPU affinity by running on per-CPU basis

**Priority Demotion (mlfq_demote):**
- Moves process from current level to next lower level
- Resets `ticks_in_queue` to 0
- Caps at level 3 (lowest priority)

**Quantum Check (mlfq_exceeded_quantum):**
- Compares `ticks_in_queue` against level-specific time quantum
- Returns true if process has used full quantum

**Priority Boost (mlfq_boost_all):**
- Resets all active processes to queue level 0
- Resets `ticks_in_queue` to 0
- Occurs every BOOST_INTERVAL ticks (256 ticks = ~26ms at typical frequency)

**Yield Enhancement:**
- Increments `ticks_in_queue` on each yield
- Checks if quantum exceeded and demotes if necessary
- Called on every timer interrupt (typically 100+ times per second)

#### 3.3 Configuration Parameters (kernel/param.h)

```c
#define MLFQ_LEVELS       4    // Number of priority queues
#define BOOST_INTERVAL    256  // Priority boost every 256 ticks
#define TIME_QUANTA_0     4    // Queue 0 time quantum
#define TIME_QUANTA_1     8    // Queue 1 time quantum
#define TIME_QUANTA_2     16   // Queue 2 time quantum
#define TIME_QUANTA_3     32   // Queue 3 time quantum
```

### 4. System Call: getprocinfo

**Purpose**: Query MLFQ scheduler state for a process

**Signature:**
```c
int getprocinfo(int pid, struct procinfo *info);
```

**Parameters:**
- `pid`: Process ID to query
- `info`: Pointer to procinfo structure (filled by syscall)

**Return Value:**
- 0 on success
- -1 if process not found or error

**procinfo Structure:**
```c
struct procinfo {
  int pid;                 // Process ID
  int queue_level;         // Current priority queue (0-3)
  uint64 ticks_in_queue;   // Ticks in current queue
  uint64 total_ticks;      // Total ticks consumed
  int state;               // Process state
  char name[16];           // Process name
};
```

### 5. Process Behavior Classification

The MLFQ scheduler automatically classifies processes based on their CPU behavior:

**I/O-Bound Processes:**
- Yield CPU before time quantum expires
- Remain at high priority queues
- Fast response time for I/O completion
- Example: shell, interactive applications

**CPU-Bound Processes:**
- Use entire time quantum before yielding
- Gradually demoted to lower priority queues
- Better fairness for other processes
- Example: computational tasks, background jobs

**Mixed Behavior:**
- Start at high priority for quick response
- If no I/O after quantum, demoted to lower priority
- Can return to high priority after boost interval
- Example: most real applications

### 6. Performance Characteristics

**Advantages:**
1. **Interactivity**: I/O-bound processes get low-latency responses
2. **Fairness**: CPU-bound processes eventually get scheduled
3. **No Starvation**: Boost mechanism ensures all processes make progress
4. **Adaptation**: Automatic classification of process behavior
5. **Efficiency**: Longer time slices for CPU-bound work reduce context switches

**Trade-offs:**
1. **Complexity**: More complex than simple round-robin
2. **Tuning**: Requires careful selection of time quanta and boost interval
3. **Overhead**: Additional per-process state tracking

### 7. Testing Strategy

**Test 1: CPU-Bound Process**
- Single process that runs tight loop
- Observe demotion to lower priority queues
- Track ticks_in_queue reaching time quantum limits

**Test 2: I/O-Bound Process**
- Process that yields frequently via pause()
- Should remain at high priority level
- Quick response to I/O completion

**Test 3: Multiple Processes**
- Multiple CPU-bound processes
- Verify round-robin at same priority
- Monitor priority boost effects

**Test 4: Starvation Prevention**
- Long-running low-priority process
- Verify boost restores to priority 0
- Ensure regular CPU allocation

### 8. Files Modified

**Kernel Files:**
- `kernel/proc.h`: Added MLFQ fields to struct proc
- `kernel/proc.c`: Implemented MLFQ scheduler and helpers
- `kernel/param.h`: Added MLFQ configuration parameters
- `kernel/syscall.h`: Added SYS_getprocinfo
- `kernel/syscall.c`: Added syscall table entry
- `kernel/sysproc.c`: Implemented sys_getprocinfo
- `kernel/procinfo.h`: Added procinfo structure

**User Files:**
- `user/user.h`: Added getprocinfo declaration
- `user/procinfo.h`: Added procinfo structure for user programs
- `user/usys.pl`: Added getprocinfo syscall stub
- `user/mlfq_test.c`: Comprehensive test program
- `Makefile`: Added mlfq_test to UPROGS

### 9. Kernel Parameters Tuning

The following parameters can be adjusted in `kernel/param.h` to tune scheduler behavior:

**BOOST_INTERVAL (default 256):**
- Increase: Allows more differentiation between CPU-bound and I/O-bound, but risks starvation
- Decrease: More frequent boosts provide better fairness for low-priority processes

**TIME_QUANTA_X (default 4, 8, 16, 32):**
- Increase all: Fewer context switches, higher latency
- Decrease all: More context switches, lower latency

### 10. Future Enhancements

1. **Dynamic Time Quanta**: Adjust based on system load
2. **Priority Decay**: Gradual priority decay instead of fixed levels
3. **Per-CPU Queues**: Reduce lock contention on multi-core systems
4. **Aging**: Age processes to prevent indefinite starvation
5. **Accounting**: Separate I/O time from CPU time accounting

### 11. Conclusion

The MLFQ scheduler implementation provides xv6 with adaptive process scheduling that balances responsiveness for interactive processes with fairness for CPU-bound workloads. The design is based on established operating systems principles (OSTEP) and includes starvation prevention to ensure all processes make progress.
