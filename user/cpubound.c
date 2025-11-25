#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    // Busy loop to consume CPU time continuously.
    volatile uint64 i = 0;

    while(1) {
        i++;
        // Optional: print something occasionally
        if(i % 100000000 == 0) {
            printf("CPU...\n");
        }
    }

    exit(0);
}