#define NPROC        10  // maximum number of processes
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define NDEV         10  // maximum major device number
#define ROOTDEV       0  // device number of file system root disk
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define NTRANS     2
#define MAXOPS   5   // max syscalls per transaction (before triggering commit)
#define TRANSSIZE    (MAXOPBLOCKS * MAXOPS)  // 5 syscalls and leave room for 2 blocks for desc/commit blocks
#define LOGSIZE      200  // max data blocks in on-disk log
#define NBUF         200  // size of disk block cache
#define FSSIZE       200000  // size of file system in blocks
#define MAXPATH      128   // maximum file path name
#define NDISK        2