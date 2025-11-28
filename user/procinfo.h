// Process information structure for user-space programs
// Used with getprocinfo() syscall

#ifndef USER_PROCINFO_H
#define USER_PROCINFO_H

struct procinfo {
  int pid;                 // Process ID
  int queue_level;         // Current priority queue (0=highest, 3=lowest)
  uint64 ticks_in_queue;   // Ticks spent in current queue
  uint64 total_ticks;      // Total ticks consumed
  int state;               // Process state (1=USED, 2=SLEEPING, 3=RUNNABLE, 4=RUNNING, 5=ZOMBIE)
  char name[16];           // Process name
};

#define PROCSTATE_UNUSED    0
#define PROCSTATE_USED      1
#define PROCSTATE_SLEEPING  2
#define PROCSTATE_RUNNABLE  3
#define PROCSTATE_RUNNING   4
#define PROCSTATE_ZOMBIE    5

#endif
