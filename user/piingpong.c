#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
  int p1[2];  // parent -> child
  int p2[2];  // child -> parent
  pipe(p1);
  pipe(p2);

  int pid = fork();

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // Child process
    close(p1[1]); // child doesn’t write to p1
    close(p2[0]); // child doesn’t read from p2

    for (int i = 0; i < 10; i++) 
    {
      char buf;
      read(p1[0], &buf, 1); // receive from parent
      printf("Child received: %c (iteration %d)\n", buf, i + 1);
      write(p2[1], "x", 1); // send back
    }

    close(p1[0]);
    close(p2[1]);
    exit(0);

  } 
  else 
  {
    // Parent process
    close(p1[0]); // parent doesn’t read from p1
    close(p2[1]); // parent doesn’t write to p2

    for (int i = 0; i < 10; i++) 
    {
      write(p1[1], "p", 1); // send to child
      char buf;
      read(p2[0], &buf, 1); // wait for reply
      printf("Parent received: %c (iteration %d)\n", buf, i + 1);
    }

    close(p1[1]);
    close(p2[0]);
    wait(0);
  }

  exit(0);
}