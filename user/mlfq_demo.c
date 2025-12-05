#include "kernel/types.h"
#include "user/user.h"

// Structure for getprocinfo - matches kernel implementation
struct procinfo {
  int pid;
  int state;
  int queue_level;
  uint64 ticks_in_queue;
};

int main(int argc, char *argv[]) {
  int pid;
  
  printf("\n=== MLFQ Scheduler Demo ===\n\n");
  
  printf("Test 1: CPU-bound process\n");
  pid = fork();
  
  if(pid == 0) {
    // Child: CPU-bound work - long enough to accumulate many ticks
    struct procinfo info;
    printf("Child running CPU-bound task...\n");
    
    volatile long sum = 0;
    // Do CPU work with frequent checkpoints to observe queue progression
    // Using smaller loop sizes so we can see transitions between checkpoints
    for(int loop = 0; loop < 200; loop++) {
      // Smaller computation between checkpoints (~5-10 timer ticks worth)
      for(long i = 0; i < 5000000; i++) {
        sum += i;
        sum = sum % 1000000;  // Keep it bounded
      }
      
      // Check status at each checkpoint to see progression
      if(getprocinfo((uint64)&info) == 0) {
        printf("  Checkpoint %d - Queue: %d, Ticks: %ld\n", loop, info.queue_level, info.ticks_in_queue);
      }
    }
    
    // Final check
    if(getprocinfo((uint64)&info) == 0) {
      printf("  Final - Queue: %d, Ticks: %ld\n", info.queue_level, info.ticks_in_queue);
    }
    exit(0);
  } else {
    wait(0);
    printf("CPU-bound test complete\n\n");
  }
  
  printf("Test 2: I/O-bound process\n");
  pid = fork();
  
  if(pid == 0) {
    // Child: I/O-bound work - frequent yields to stay high priority
    struct procinfo info;
    printf("Child running I/O-bound task...\n");
    
    // Do many iterations with frequent I/O (pause calls)
    // Should stay at Queue 0 because it yields before quantum expires
    for(int i = 0; i < 50; i++) {
      pause(10);  // Yield for 10 ticks, then return to work
      
      // Check status at each checkpoint to verify staying at Q0
      if(i % 10 == 0) {
        if(getprocinfo((uint64)&info) == 0) {
          printf("  Checkpoint %d - Queue: %d, Ticks: %ld\n", i, info.queue_level, info.ticks_in_queue);
        }
      }
    }
    
    // Final check
    if(getprocinfo((uint64)&info) == 0) {
      printf("  Final - Queue: %d, Ticks: %ld\n", info.queue_level, info.ticks_in_queue);
    }
    exit(0);
  } else {
    wait(0);
    printf("I/O-bound test complete\n\n");
  }
  
  printf("=== Demo Complete ===\n\n");
  exit(0);
}
