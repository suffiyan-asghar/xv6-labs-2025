// Process information for getprocinfo syscall
// Used to query MLFQ scheduler state

struct procinfo {
  int pid;                 // Process ID
  int queue_level;         // Current priority queue (0=highest, 3=lowest)
  uint64 ticks_in_queue;   // Ticks spent in current queue
  uint64 total_ticks;      // Total ticks consumed
  int state;               // Process state
  char name[16];           // Process name
};
