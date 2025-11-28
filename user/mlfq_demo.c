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
    // Child: CPU-bound work
    struct procinfo info;
    printf("Child running CPU-bound task...\n");
    
    volatile int sum = 0;
    for(int i = 0; i < 3000000; i++) {
      sum += i;
    }
    
    // Check own info before exit
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
    // Child: I/O-bound work
    struct procinfo info;
    printf("Child running I/O-bound task...\n");
    
    for(int i = 0; i < 15; i++) {
      pause(10);  // Yield frequently
    }
    
    // Check own info before exit
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
