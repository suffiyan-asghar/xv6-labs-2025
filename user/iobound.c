#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    while(1){
        printf("IO...\n");
        pause(5);      // pause for 5 "ticks" (your xv6 uses this instead of sleep)
    }

    exit(0);
}