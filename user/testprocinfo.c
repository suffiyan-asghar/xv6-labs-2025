#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  struct procinfo pi;

  int pid = getpid();
  getprocinfo(pid, &pi);

  printf("PID=%d\nState=%d\nTicks=%d\nQueue Level=%d\n",
         pi.pid, pi.state, pi.ticks, pi.queue_level);

  exit(0);
}
