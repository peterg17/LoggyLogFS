#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "log.h"


/*
An xv6 implementation of an EXT3-like logging system 
that allows for several types of concurrency:

  1. FS syscall concurrency: like the standard xv6 logging scheme,
     we allow several syscalls to modify blocks at the same 

  2. Transaction concurrency: can write to the current transaction
      in memory while other transactions are committing

*/

struct log log[NDISK];

static void recover_from_log(int);
static void commit(int);

void
initlog(int dev, struct superblock *sb)
{
  printf("initializing log...\n");
  initlock(&log[dev].lock, "log");
  for(int i = 0; i < MAXTRANS; i++) {
    initlock(&log[dev].transactions[i].lock, "txn");
  }
  log[dev].start = sb->logstart;
  log[dev].size = sb->nlog;
  log[dev].dev = dev;
}

// Copy committed blocks from log to their home location
static void
install_trans(int dev)
{
  panic("install trans not implemented\n");
}

// Read the log header from disk into the in-memory log header
static void
read_head(int dev)
{
  panic("read head not implemented\n");
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(int dev)
{
  panic("write head not implemented\n");
}

static void
recover_from_log(int dev)
{
  panic("recover from log not implemented\n");
}

// called at the start of each FS system call.
void
begin_op(int dev)
{
  int currTransIdx;
  struct transaction *currTrans;

  printf("in begin op\n");
  acquire(&log[dev].lock);
  currTransIdx = &log[dev].transidx;
  currTrans = &log[dev].transactions[currTransIdx];
  // do we also need to acquire transaction lock?
  acquire(&currTrans->lock);

  while(1){
    // check if this will make the transaction full if not
    if(currTrans->blocksWritten + ((currTrans->outstanding + 1)*MAXOPBLOCKS) > TRANSIZE) {
      sleep(&log, &log[dev].lock);
    } else {
      currTrans->outstanding += 1;
      release(&currTrans->lock);
      release(&log[dev].lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(int dev)
{
  panic("end op not implemented\n");
}

// Copy modified blocks from cache to log.
static void
write_log(int dev)
{
  panic("write log not implemented\n");
}

static void
commit(int dev)
{
  panic("haven't implemented commit\n");
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  panic("haven't implemented log_write\n");
}


