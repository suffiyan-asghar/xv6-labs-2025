# MLFQ Core Concepts - Q&A for Understanding

## Quick Reference Guide

### Q1: What are the three global MLFQ data structures and what do they do?

**Answer:**
```c
struct spinlock mlfq_lock;              // Protects MLFQ state from race conditions
struct proc *mlfq_queues[MLFQ_LEVELS];  // Array of 4 queue heads (Q0, Q1, Q2, Q3)
uint64 last_boost_ticks = 0;            // Tracks when last priority boost occurred
```

**Explanation:**
- `mlfq_lock` - Spinlock ensuring only one CPU modifies MLFQ at a time (mutual exclusion)
- `mlfq_queues[]` - 4 pointers to process queues (each represents a priority level)
- `last_boost_ticks` - Global counter to track boost timing (prevents starvation)

---

### Q2: What does `yield()` do and why is it important?

**Answer:**
```c
void yield(void) {
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;    // Signal: "I'm giving up CPU"
  sched();                 // Switch control to scheduler
  release(&p->lock);
}
```

**What it does:**
1. Process voluntarily gives up CPU
2. Sets state to `RUNNABLE` (not `RUNNING`)
3. Calls `sched()` to transfer control back to scheduler
4. Scheduler picks next process

**When used:**
- Process waits for I/O (disk, network)
- Process calls `sleep()`
- Process completes work early
- Timer interrupt ends time quantum

---

### Q3: Why is MLFQ demotion logic in `trap.c` (timer interrupt handler) instead of `yield()`?

**Answer:**

**The Problem:** If demotion only happened in `yield()`, CPU-bound processes that never yield wouldn't be demoted.

```
CPU-bound process: Runs continuously, never yields
→ Would stay in Q0 forever (starvation!)

I/O-bound process: Yields frequently 
→ Stays in Q0 (correct)
```

**The Solution:** Handle demotion on every timer interrupt

```c
// In trap.c - called EVERY timer tick
if(which_dev == 2) {                    // Timer interrupt
  p->ticks_in_queue++;                  // Count ticks
  p->total_ticks++;
  
  // Check if exceeded quantum
  if(p->ticks_in_queue >= quantum && p->queue_level < MLFQ_LEVELS - 1) {
    p->queue_level++;                   // DEMOTE
    p->ticks_in_queue = 0;              // Reset counter
  }
  yield();                              // Force yield after update
}
```

**Key Insight:** Timer interrupt fires **regardless of process behavior**, so:
- CPU-bound process that doesn't yield? → Timer tracks it → Gets demoted
- I/O-bound process that yields early? → Timer sees low ticks → Stays in Q0

**Example Timeline (Q0 quantum = 4 ticks):**
```
Tick 0: Process starts running
  → ticks_in_queue = 1 (after interrupt)
  
Tick 1: Timer fires again
  → ticks_in_queue = 2
  
Tick 2: Timer fires
  → ticks_in_queue = 3
  
Tick 3: Timer fires
  → ticks_in_queue = 4
  → Equals quantum! Check: >= 4? YES
  → DEMOTE to Q1, reset counter to 0
  
Tick 4: Now in Q1 with 8-tick quantum
```

---

### Q4: What does `mlfq_boost_all()` do and why is it needed?

**Answer:**

```c
static void mlfq_boost_all(void) {
  struct proc *p;
  extern uint ticks;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state != UNUSED && p->state != ZOMBIE) {
      p->queue_level = 0;           // Reset to Q0 (highest)
      p->ticks_in_queue = 0;        // Reset counter
      p->last_boost_ticks = ticks;
    }
  }
}
```

**What it does:**
- Called every `BOOST_INTERVAL` ticks (default 256)
- **Resets ALL active processes back to Q0**
- Clears their `ticks_in_queue` counters

**Why it's needed - Starvation Prevention:**

Without boosting:
```
Q0: [shell, editor]  ← Always have work, keep yielding before quantum
Q1: [background]
Q2: [cleanup]
Q3: [batch job]      ← STARVES! Never runs!

Why? Scheduler always picks from Q0 first.
Q0 always has runnable processes.
Q3 never gets a turn.
```

With boosting (every 256 ticks):
```
BEFORE BOOST (at tick 256):
  Q0: [shell, editor]
  Q1: [background] waiting
  Q2: [cleanup] waiting
  Q3: [batch job] waiting 256 ticks!

mlfq_boost_all() called:

AFTER BOOST:
  Q0: [shell, editor, background, cleanup, batch job]
  Q1: empty
  Q2: empty
  Q3: empty

Now batch job gets its turn! Makes progress!
If it uses full quantum, gets demoted again.
But it's not starved anymore.
```

---

### Q5: What's the difference between CPU-bound and I/O-bound processes?

**Answer:**

| Characteristic | CPU-Bound | I/O-Bound |
|---|---|---|
| **Definition** | Continuously uses CPU, minimal I/O | Frequently waits for I/O, yields often |
| **Example** | `while(1) sum++;` | Reading from disk, network request |
| **Behavior** | Uses full time quantum | Yields before quantum exhausted |
| **Queue Evolution** | Q0 → Q1 → Q2 → Q3 | Stays at Q0 |
| **Responsiveness** | Slow (waits in lower queues) | Fast (always in Q0) |
| **Fair Share** | Low priority but guaranteed by boost | High priority |

**CPU-Bound Timeline:**
```
Created: Q0, ticks_in_queue=0
After 4 ticks: Uses full quantum
  → Demoted to Q1, ticks_in_queue=0
After 8 more ticks: Uses full quantum
  → Demoted to Q2, ticks_in_queue=0
After 16 more ticks: Uses full quantum
  → Demoted to Q3, ticks_in_queue=0
After 32 ticks at Q3: Still using CPU
  → Stays at Q3 until boost interval
  
Total: 28 ticks to reach Q3
```

**I/O-Bound Timeline:**
```
Created: Q0, ticks_in_queue=0
After 1 tick: Needs disk → Calls yield()
  → ticks_in_queue reset to 0
  → Stays at Q0 (never reached quantum of 4)
  
Disk ready, scheduler runs it again at Q0
After 2 ticks: Needs network → Calls yield()
  → Stays at Q0
  
Pattern continues: Always yields before 4-tick quantum
  → Forever stays in Q0
  → Always runs with minimal latency
```

**Key Insight:** The scheduler **never demotes** I/O-bound processes because they never use their full time quantum. They voluntarily yield first.

---

### Q6: Why does only the scheduler scan Q0 → Q1 → Q2 → Q3?

**Answer:**

Scheduler code:
```c
// Search from highest to lowest priority
for(int level = 0; level < MLFQ_LEVELS; level++) {
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state == RUNNABLE && p->queue_level == level) {
      // Found runnable process at this level
      // Run it!
    }
  }
  // If found someone at this level, stop searching
  if(best != 0) break;
}
```

**Logic:**
1. Scan Q0 for runnable processes → If found, run it
2. Only scan Q1 if **no process in Q0 is runnable**
3. Only scan Q2 if Q0 and Q1 are empty
4. Only scan Q3 if Q0, Q1, Q2 are all empty

**Why?** Priority enforcement - higher priority processes should run first

**Consequence - Why boosting is essential:**
```
Without boosting:
- Q0 processes always available (shell, editor, background tasks)
- Scheduler always finds something in Q0
- Never needs to check Q1, Q2, Q3
- Processes demoted to Q3 never run (STARVED!)

With boosting (every 256 ticks):
- Q3 process reset to Q0 during boost
- Gets a turn to run in Q0
- Makes progress
- Then demoted back down
- Never starves for 256+ ticks
```

**Example Scheduling Sequence:**
```
Time 0-10:    Q0 has [shell, editor]
              Q1 has [bg_task]
              Q2 has [cleanup]
              Q3 has [batch_job]
              
Scheduler runs: shell → editor → shell → editor
Never touches Q3!

Time 256:     mlfq_boost_all() called
              All processes now in Q0
              
Scheduler now picks: shell → editor → bg_task → cleanup → batch_job
Now batch_job gets to run!
```

---

### Q7: What are the 4 MLFQ fields in each process struct?

**Answer:**

```c
struct proc {
  // ... other fields ...
  int queue_level;           // Current priority: 0 (highest) to 3 (lowest)
  uint64 ticks_in_queue;     // Time spent in current queue
  uint64 total_ticks;        // Total CPU time used by this process
  uint64 last_boost_ticks;   // When process was last boosted to Q0
};
```

**What each tracks:**

| Field | Purpose | Example |
|---|---|---|
| `queue_level` | Current priority level | Process at Q2 = lower priority than Q0 |
| `ticks_in_queue` | Time used in current queue | Q0: 4 ticks max before demotion |
| `total_ticks` | Cumulative CPU time | How much total CPU has process used? |
| `last_boost_ticks` | Last reset time | Helps determine if process was recently boosted |

**Initialization:**
```c
// In procinit() - when kernel starts
p->queue_level = 0;      // Everyone starts at Q0
p->ticks_in_queue = 0;
p->total_ticks = 0;
p->last_boost_ticks = 0;

// In allocproc() - when new process created
p->queue_level = 0;      // New jobs enter at highest priority
p->ticks_in_queue = 0;
p->total_ticks = 0;
```

---

### Q8: What happens at each timer interrupt?

**Answer:**

```c
// In usertrap() or kerneltrap() when timer interrupt fires
if(which_dev == 2) {  // Timer interrupt
  acquire(&p->lock);
  
  // 1. Update time counters
  p->ticks_in_queue++;     // Increment ticks in current queue
  p->total_ticks++;        // Increment total ticks
  
  // 2. Get time quantum for current level
  uint64 quantum;
  switch(p->queue_level) {
    case 0: quantum = TIME_QUANTA_0; break;  // 4 ticks
    case 1: quantum = TIME_QUANTA_1; break;  // 8 ticks
    case 2: quantum = TIME_QUANTA_2; break;  // 16 ticks
    case 3: quantum = TIME_QUANTA_3; break;  // 32 ticks
  }
  
  // 3. Check if exceeded quantum
  if(p->ticks_in_queue >= quantum && p->queue_level < MLFQ_LEVELS - 1) {
    p->queue_level++;      // Demote to lower priority
    p->ticks_in_queue = 0; // Reset counter
  }
  
  release(&p->lock);
  yield();  // Force process to yield to scheduler
}
```

**Step-by-step for one process:**
```
Tick 0: Enter Q0, ticks=0
  Timer: ticks=1, check: 1>=4? No, stay in Q0
  
Tick 1: Still in Q0, ticks=1
  Timer: ticks=2, check: 2>=4? No, stay in Q0
  
Tick 2: Still in Q0, ticks=2
  Timer: ticks=3, check: 3>=4? No, stay in Q0
  
Tick 3: Still in Q0, ticks=3
  Timer: ticks=4, check: 4>=4? YES! Demote!
    queue_level = 1, ticks_in_queue = 0
  
Tick 4: Now in Q1, ticks=0
  Timer: ticks=1, check: 1>=8? No, stay in Q1
  
... continues for Q1 with 8-tick quantum ...
```

---

### Q9: How is the scheduler called and what does it do?

**Answer:**

```c
void scheduler(void) {
  // Infinite loop - scheduler never returns
  for(;;) {
    intr_on();    // Enable interrupts
    intr_off();   // Disable to avoid race
    
    acquire(&mlfq_lock);
    
    // 1. Check if time for priority boost
    if(ticks - last_boost_ticks >= BOOST_INTERVAL) {
      mlfq_boost_all();          // Reset all to Q0
      last_boost_ticks = ticks;
    }
    release(&mlfq_lock);
    
    // 2. Find highest priority RUNNABLE process
    struct proc *best = 0;
    for(int level = 0; level < MLFQ_LEVELS; level++) {
      for(p = proc; p < &proc[NPROC]; p++) {
        if(p->state == RUNNABLE && p->queue_level == level) {
          best = p;
          break;  // Found someone in this level
        }
      }
      if(best != 0) break;  // Stop searching
    }
    
    // 3. Run the process
    if(best != 0) {
      best->state = RUNNING;
      swtch(&c->context, &best->context);  // Switch to process
      // ... process runs until timer interrupt or yields ...
      // ... swtch returns here when process yields ...
      best->state = RUNNABLE;
    } else {
      asm volatile("wfi");  // Wait if nothing to run
    }
  }
}
```

**Scheduling sequence:**
```
Scheduler loop iteration 1:
  Check: Time for boost? No (ticks - last_boost = 50)
  Scan Q0: Find shell (RUNNABLE)
  Set shell → RUNNING
  swtch() to shell
  
Shell runs for a while, then:
  Timer interrupt fires
  trap.c: ticks_in_queue++, yield()
  yield() calls sched()
  
Scheduler loop iteration 2:
  Check: Time for boost? No
  Scan Q0: Find editor (RUNNABLE)
  Set editor → RUNNING
  swtch() to editor
  
... repeats ...

Scheduler loop iteration N (at tick 256):
  Check: Time for boost? YES (ticks - last_boost = 256)
  mlfq_boost_all(): All processes → Q0
  last_boost_ticks = 256
  Scan Q0: Find all processes!
  Run them in order
```

---

### Q10: Complete example - CPU-bound vs I/O-bound scheduling

**Answer:**

**Scenario:** Two processes compete
- Process A: CPU-bound (sum += i)
- Process B: I/O-bound (read disk)

**Timeline:**

```
Time 0: Both created
  A: Q0, ticks_in_queue=0, total_ticks=0
  B: Q0, ticks_in_queue=0, total_ticks=0

Time 0-3: Scheduler picks A (first in table)
  Tick 1: A ticks_in_queue=1, total_ticks=1 (runs)
  Tick 2: A ticks_in_queue=2, total_ticks=2 (runs)
  Tick 3: A ticks_in_queue=3, total_ticks=3 (runs)
  Tick 4: A ticks_in_queue=4, total_ticks=4
          CHECK: 4>=4? YES → Demote A to Q1, ticks=0
          yield() forces A to give up CPU

Time 4-7: Scheduler picks B (now A in Q1, B in Q0)
  Tick 5: B ticks_in_queue=1, total_ticks=1
          B needs disk I/O → sleep()
          B state becomes SLEEPING
          B doesn't use full quantum!

Time 5-8: B waiting for disk
  Scheduler looks: Q0 empty, Q1 has A
  Tick 6: A (in Q1) ticks_in_queue=1, total_ticks=5 (runs)
  Tick 7: A ticks_in_queue=2, total_ticks=6
  Tick 8: A ticks_in_queue=3, total_ticks=7

Time 8-11: B disk ready, back to RUNNABLE at Q0
  Scheduler looks: Q0 has B, Q1 has A
  Q0 has higher priority → Pick B
  Tick 9: B ticks_in_queue=1, total_ticks=2
          B reads more data → sleep()
          B didn't use full quantum again!

Time 9-14: B sleeping, A gets CPU
  Tick 10: A (Q1) ticks_in_queue=4, total_ticks=8
  Tick 11: A ticks_in_queue=5, total_ticks=9
  Tick 12: A ticks_in_queue=6, total_ticks=10
  Tick 13: A ticks_in_queue=7, total_ticks=11
  Tick 14: A ticks_in_queue=8, total_ticks=12
           CHECK: 8>=8 (Q1 quantum)? YES → Demote A to Q2
           A: queue_level=2, ticks_in_queue=0

AFTER MANY ITERATIONS:
  A: Q3, total_ticks=100+ (low priority)
  B: Q0, total_ticks=50 (high priority, responsive)
  
Scheduling favors B because:
  - B yields early, stays in Q0
  - A uses full quantum, keeps demoting
  - B gets more CPU time overall despite lower total_ticks
```

**Why this is good:**
- Shell/editor (like B) stay responsive
- Batch jobs (like A) don't starve, just get less priority
- Fair: Both eventually run, B just runs more frequently

---

## Quick Quiz - Test Your Understanding

**Question 1:** If a process never yields and just loops, how does the MLFQ scheduler eventually lower its priority?
<details>
<summary>Answer</summary>
Timer interrupts fire every tick. Even if process doesn't yield, the timer handler in trap.c tracks ticks_in_queue and demotes when it exceeds the quantum. This happens automatically.
</details>

**Question 2:** Why would a low-priority process in Q3 starve forever without boosting?
<details>
<summary>Answer</summary>
Scheduler only checks Q3 if Q0, Q1, Q2 are all empty. If Q0 always has runnable processes, Q3 never runs. Boosting resets Q3 processes to Q0 so they get turns.
</details>

**Question 3:** What happens if a process yields before using its full quantum?
<details>
<summary>Answer</summary>
It stays in the same queue level. ticks_in_queue doesn't reach the quantum threshold, so no demotion occurs. This is how I/O-bound processes stay in Q0.
</details>

**Question 4:** When mlfq_boost_all() is called, does it immediately run all boosted processes?
<details>
<summary>Answer</summary>
No. It just resets their queue_level to 0 and ticks_in_queue to 0. The scheduler still picks which one to run next based on scan order.
</details>

**Question 5:** If TIME_QUANTA_0 = 4 and a process uses exactly 4 ticks, is it demoted?
<details>
<summary>Answer</summary>
Yes. The check is `ticks_in_queue >= quantum`, so 4 >= 4 is true. Process gets demoted after using full quantum.
</details>

---

## Key Takeaways

1. ✅ **MLFQ tracks behavior automatically** - No manual priority specification
2. ✅ **Timer interrupts drive demotion** - Every tick is counted in trap.c
3. ✅ **Starvation fixed by boosting** - All processes reset to Q0 every 256 ticks
4. ✅ **I/O-bound stay high priority** - They yield early, never exceed quantum
5. ✅ **CPU-bound get demoted** - They use full quantum, move down queues
6. ✅ **Scheduler always picks Q0 first** - Strict priority enforcement
7. ✅ **No process starves forever** - Boost interval guarantees progress
