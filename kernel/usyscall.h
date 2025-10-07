#ifndef _USYSCALL_H_
#define _USYSCALL_H_

#include "memlayout.h"

// This structure holds the data shared between the kernel and userspace
struct usyscall {
    int pid; // The PID of the current process
};

// USYSCALL is the virtual address where the read-only usyscall page is mapped.
// It is placed just below the TRAPFRAME.
#define USYSCALL (TRAPFRAME - PGSIZE)

#endif // _USYSCALL_H_