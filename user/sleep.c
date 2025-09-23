// user/sleep.c
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "Usage: sleep <ticks>\n");
    exit(1);
  }

  int ticks = atoi(argv[1]);
  if(ticks < 0){
    fprintf(2, "sleep: ticks must be non-negative\n");
    exit(1);
  }

  // call the system call to pause for the given number of ticks
  if(pause(ticks) < 0){
    fprintf(2, "sleep: pause syscall failed\n");
    exit(1);
  }

  exit(0);
}
