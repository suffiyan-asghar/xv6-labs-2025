
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(2, "usage: sleep ticks\n");
    exit(1);
  }

  int ticks = atoi(argv[1]);
  if (ticks < 0) {
    fprintf(2, "sleep: ticks must be non-negative\n");
    exit(1);
  }

  // pause for 'ticks' timer ticks
  if (pause(ticks) < 0) {
    // (pause returns -1 on error)
    fprintf(2, "sleep: pause failed\n");
    exit(1);
  }

  exit(0);
}
