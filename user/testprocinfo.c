#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Simple structure matching the kernel's getprocinfo output
struct procinfo {
  int pid;
  int state;
  int queue_level;
  uint64 ticks_in_queue;
};

int main(int argc, char *argv[]) {
  struct procinfo info;

  printf("=== Simple getprocinfo Test ===\n");
  printf("Testing current process (PID %d)...\n\n", getpid());

  if(getprocinfo((uint64)&info) < 0) {
    printf("ERROR: getprocinfo failed!\n");
    exit(1);
  }

  printf("SUCCESS! Process info retrieved:\n");
  printf("  PID:              %d\n", info.pid);
  printf("  Queue Level:      %d (0=highest, 3=lowest)\n", info.queue_level);
  printf("  Ticks in Queue:   %ld\n", info.ticks_in_queue);
  printf("  State:            %d\n", info.state);
  
  printf("\n=== Test Complete ===\n");
  exit(0);
}