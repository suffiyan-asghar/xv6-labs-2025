#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  struct procinfo pi;

  int pid = getpid();

  if(getprocinfo(pid, &pi) < 0){
    printf("getprocinfo failed\n");
    exit(1);
  }

  printf("Process Info:\n");
  printf("pid=%d state=%d\n", pi.pid, pi.state);

  exit(0);
}
