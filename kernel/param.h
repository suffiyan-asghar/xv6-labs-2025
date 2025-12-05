#define NPROC        64  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGBLOCKS    (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       2000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define USERSTACK    1     // user stack pages

// MLFQ Scheduler parameters
#define MLFQ_LEVELS  4     // Number of priority queue levels
#define BOOST_INTERVAL 50 // Priority boost interval (ticks) - lowered for testing visibility
// Time quanta for each queue level (in ticks)
// Queue 0 (highest priority): 4 ticks
// Queue 1: 8 ticks
// Queue 2: 16 ticks
// Queue 3 (lowest priority): 32 ticks
#define TIME_QUANTA_0 4
#define TIME_QUANTA_1 8
#define TIME_QUANTA_2 16
#define TIME_QUANTA_3 32

