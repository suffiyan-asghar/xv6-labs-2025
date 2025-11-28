# MLFQ Scheduler - Implementation Notes

## Code Changes Summary

### 1. kernel/param.h
Added MLFQ configuration parameters:
```c
#define MLFQ_LEVELS  4
#define BOOST_INTERVAL 256
#define TIME_QUANTA_0 4
#define TIME_QUANTA_1 8
#define TIME_QUANTA_2 16
#define TIME_QUANTA_3 32
```

### 2. kernel/proc.h
Extended struct proc with MLFQ fields:
```c
int queue_level;           // 0=highest, 3=lowest
uint64 ticks_in_queue;     // Time at current level
uint64 total_ticks;        // Total CPU time
uint64 last_boost_ticks;   // Last boost time
```

### 3. kernel/proc.c

#### Global Variables
```c
struct spinlock mlfq_lock;
struct proc *mlfq_queues[MLFQ_LEVELS];
uint64 last_boost_ticks = 0;
```

#### Helper Functions
- `get_time_quantum(int level)` - Returns time slice for queue level
- `mlfq_demote(struct proc *p)` - Move process to lower priority
- `mlfq_exceeded_quantum(struct proc *p)` - Check if time quantum exceeded
- `mlfq_boost_all()` - Reset all processes to top queue

#### Modified Functions
- `procinit()` - Initialize MLFQ queues and lock
- `allocproc()` - Set MLFQ fields for new process (queue_level=0)
- `freeproc()` - Reset MLFQ fields
- `yield()` - Track ticks and perform demotion
- `scheduler()` - New MLFQ-aware scheduler implementation

### 4. kernel/procinfo.h (NEW)
Kernel-side process info structure for getprocinfo syscall.

### 5. kernel/syscall.h
Added: `#define SYS_getprocinfo 22`

### 6. kernel/syscall.c
- Added `extern uint64 sys_getprocinfo(void);`
- Added entry to syscalls array

### 7. kernel/sysproc.c
Implemented `sys_getprocinfo()`:
- Finds process by PID
- Collects MLFQ state and basic info
- Copies to user space via copyout

### 8. user/user.h
Added: `int getprocinfo(int, struct procinfo*);`

### 9. user/procinfo.h (NEW)
User-space version of procinfo structure with state constants.

### 10. user/usys.pl
Added: `entry("getprocinfo");` to generate syscall stub.

### 11. user/mlfq_demo.c (NEW)
Simple demonstration program showing:
- CPU-bound process demotion
- I/O-bound process staying high priority

### 12. user/mlfq_test.c (NEW)
Comprehensive test suite:
- Individual CPU-bound and I/O-bound tests
- Multiple process interaction test
- Priority boost verification

### 13. Makefile
Added `_mlfq_demo` and `_mlfq_test` to UPROGS list.

## Key Design Decisions

### 1. Lock Strategy
- Single `mlfq_lock` for priority boost checking
- Per-process `p->lock` for queue level modifications
- Avoids holding mlfq_lock during long operations

### 2. Scheduler Implementation
- Linear search through queue levels (not queue-based list)
- Simpler implementation, adequate for xv6
- Could be optimized with actual queues if needed

### 3. Time Quantum Tracking
- Incremented on every yield() (every timer interrupt)
- Not perfectly accurate but adequate for demonstration
- More accurate tracking would require modifying timer code

### 4. Starvation Prevention
- Simple boost interval instead of aging
- Easier to understand and implement
- Effective for most workloads

### 5. State Initialization
- New processes start at queue level 0 (highest priority)
- Allows quick identification of interactive processes
- Matches MLFQ Rule 3

## Potential Improvements

### 1. More Accurate Time Tracking
Current:
```c
// In yield()
p->ticks_in_queue++;
```

Better:
```c
// In trap handler, track actual CPU time
// Increment only when returning to user mode
```

### 2. Per-CPU Queues
Current: Single global scheduler
Better: Per-CPU queues with lock-free structures

### 3. Dynamic Boost Interval
```c
// Adjust BOOST_INTERVAL based on load
if(num_runnable_procs > threshold) {
  boost_interval = BOOST_INTERVAL / 2;
}
```

### 4. Smart Demotion
```c
// Don't demote processes that haven't yielded
if(p->yielded_this_quantum) {
  // Keep at current level
}
```

### 5. I/O Prediction
```c
// Track I/O patterns
if(process_does_io()) {
  // Keep high priority
}
```

## Testing Methodology

### CPU-Bound Process Expected Output
```
Initial: queue_level=0, ticks_in_queue=0
After 5 ticks: queue_level=1, ticks_in_queue=1  (exceeded 4-tick quantum)
After 12 ticks: queue_level=2, ticks_in_queue=4 (exceeded 8-tick quantum)
After 28 ticks: queue_level=3, ticks_in_queue=12 (exceeded 16-tick quantum)
After 256 ticks: queue_level=0, ticks_in_queue=0 (priority boost)
```

### I/O-Bound Process Expected Output
```
Remains at queue_level=0 throughout execution
ticks_in_queue never reaches time quantum because process yields early
```

## Debugging Tips

### 1. Add Debug Output
```c
// In yield()
if(p->queue_level != old_level) {
  printf("Process %d demoted from %d to %d\n", 
         p->pid, old_level, p->queue_level);
}
```

### 2. Monitor in GDB
```
(gdb) print p->queue_level
(gdb) print p->ticks_in_queue
(gdb) print ticks
```

### 3. Use getprocinfo in Shell Script
```bash
#!/bin/sh
while true
do
  # Run test and observe output
  mlfq_demo
  pause 100
done
```

## Performance Analysis

### Context Switches
- Queue 0: 4 ticks = ~40 context switches/second
- Queue 3: 32 ticks = ~3 context switches/second

### Scheduling Overhead
- Searching all MLFQ levels: O(n*m) where n=processes, m=levels
- Typical xv6: 64 processes, 4 levels = acceptable

### Memory Overhead
- 4 fields per process: ~32 bytes
- 64 max processes: ~2KB total

## Known Limitations

1. **Accuracy of ticks_in_queue**
   - Only incremented on yield
   - Doesn't account for preemption time
   - Adequate for education but not precise

2. **No I/O Accounting**
   - Can't distinguish blocking I/O from yield()
   - Would need kernel modification

3. **No Multi-core Optimization**
   - Single global scheduler works but not optimal
   - Lock contention on many cores

4. **Simple Boost Mechanism**
   - Resets all processes equally
   - More sophisticated aging possible

## Future Research Directions

1. Implement O(1) scheduler like Linux 2.6
2. Study effect of different boost intervals
3. Add machine learning process classification
4. Implement lottery scheduling variant
5. Compare with other scheduling algorithms
