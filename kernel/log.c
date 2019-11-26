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

// static void recover_from_log(int);
// static void commit(int);
void print_log_header(int);

void
initlog(int dev, struct superblock *sb)
{
  // TODO: figure out how to handle installing blocks,
  // idea: have a child process here that never returns it just
  // keeps spinning and checking for installing the transaction
  printf("initializing log...\n");

  // this is relevant in case we make the single header block too big
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log[dev].lock, "log");
  for(int i = 0; i < MAXTRANS; i++) {
    initlock(&log[dev].transactions[i].lock, "txn");
  }
  log[dev].start = sb->logstart;
  log[dev].size = sb->nlog;
  log[dev].dev = dev;
}

// Copy committed blocks from log to their home location
// static void
// install_trans(int dev)
// {
//   panic("install trans not implemented\n");
// }

// Read the log header from disk into the in-memory log header
// static void
// read_head(int dev)
// {
//   panic("read head not implemented\n");
// }

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
// static void
// write_head(int dev)
// {
//   panic("write head not implemented\n");
// }

// static void
// recover_from_log(int dev)
// {
//   panic("recover from log not implemented\n");
// }

// called at the start of each FS system call.
void
begin_op(int dev)
{
  int currTransIdx;
  struct transaction *currTrans;

  // printf("in begin op\n");
  acquire(&log[dev].lock);
  currTransIdx = log[dev].transcount % 2;
  currTrans = &log[dev].transactions[currTransIdx];
  release(&log[dev].lock);
  acquire(&currTrans->lock);
  

  while(1){
    // check if this will make the transaction full if not
    // TODO: do we sleep on a full transaction?? 

    // TODO: figure out how to block syscalls from end_op, where we start committing
    // we currently don't handle case where a syscall starts while the transaction has
    // started comimtting but hasn't updated its transaction counter
    if (currTrans->committing) {
      // i'm pretty sure we need to sleep on a committing transaction 
      sleep(currTrans, &currTrans->lock);
    } else if(currTrans->blocksWritten + ((currTrans->outstanding + 1)*MAXOPBLOCKS) > TRANSSIZE) {
      // Explanation: essentially what we are doing here is planning for the worst 
      // ie-> pretending that every syscall yet to complete will take MAXOPBLOCKS when they likely wont
      // we wakeup this transaction when each outstanding syscall finishes to check what actually happened
      // TODO: answer if the transaction counter would ever change between a begin_op and an end_op?
      sleep(currTrans, &currTrans->lock);
    } else {
      currTrans->outstanding += 1;
      release(&currTrans->lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(int dev)
{
  // printf("in end op function\n");

  // we're just going to change the # of outstanding syscalls
  // defer the commit to somewhere else (fsync, when txn is full)
  int currTxnIndex;
  // int isTxnFull;
  struct transaction *currtrans;
  struct logheader *snapshotLH;

  int do_commit = 0;

  acquire(&log[dev].lock);
  // TODO: replace this 2 aka magic number with the number of transactions variable
  currTxnIndex = log[dev].transcount % 2; 
  currtrans = &log[dev].transactions[currTxnIndex];
  acquire(&currtrans->lock);
  currtrans->outstanding -= 1;
  // printf("number of outstanding syscalls in txn is: %d\n", currtrans->outstanding);
  // isTxnFull = (currtrans->blocksWritten + MAXOPBLOCKS) > TRANSSIZE;

  if(currtrans->outstanding < 0) {
    panic("outstanding syscalls in txn is < 0!\n");
  } else if (currtrans->outstanding == 0 && (currtrans->blocksWritten == TRANSSIZE)) {
    printf("starting the commit process\n", currtrans->seqNum);
    // probably should do some other stuff like checking size of log and start commit
    // if txn is full, commit and stuff
    do_commit = 1;
    currtrans->committing = 1;
    // else we're gonna wakeup the log
  } else {
    wakeup(currtrans); 
  }

  // TODO: find out if we need a snapshot of the logheader
  // at the time we want to commit, because it is changing under us
  memmove(snapshotLH, &log[dev].lh, sizeof(log[dev].lh));

  release(&currtrans->lock);  
  release(&log[dev].lock);
  
  if(do_commit){
    // why do we call commit w/o holding locks?
    // can we still hold locks because we aren't sleeping during commit?
    commit(dev);
    acquire(&currtrans->lock);

    // TODO: when we are incrementing the transaction counter, make sure that there are no
    // outstanding syscalls, because that could lead to a hairy situation

    currtrans->committing = 0;
    // we have no wakeup call here because we don't sleep when committing
    // TODO: possible wakeup call on each transaction lock
    // because we can't possibly write to a transaction while its committing
    wakeup(currtrans);
    release(&currtrans->lock);
  }


}

// Copy modified blocks from cache to log.
// static void
// write_log(int dev)
// {
//   panic("write log not implemented\n");
// }

// static void
// commit(int dev)
// {
//   if (log[dev].lh.n > 0) {
//     write_log(dev);     // Write modified blocks from cache to log
//     write_head(dev);    // Write header to disk -- the real commit
//     // don't install transaction yet, defer it until on-disk log is full enough
//   }
// }

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
  // printf("in log write function\n");

  int i;
  int dev = b->dev;
  int currTransIdx = log[dev].transcount % 2;
  struct transaction *currTrans = &log[dev].transactions[currTransIdx];
  print_log_header(dev);

  if (log[dev].lh.n >= LOGSIZE || log[dev].lh.n >= log[dev].size - 1)
    panic("too big a transaction");
  if (currTrans->outstanding < 1)
    panic("log_write outside of trans");

  acquire(&log[dev].lock);
  acquire(&currTrans->lock);
  
  for (i = 0; i < log[dev].lh.n; i++) {
    if (log[dev].lh.block[i] == b->blockno)
      break;  // log absorption -- if block has changed already we're just gonna update
  }
  log[dev].lh.block[i] = b->blockno;
  if (i == log[dev].lh.n) {
    // we are at the end of the logheader, means we added new block
    bpin(b); // need to somehow make sure that we can't write 2 different versions of block across transactions
    log[dev].lh.n++;
    currTrans->blocksWritten++;
  }
  printf("blocks written to transaction: %d\n", currTrans->blocksWritten);
  release(&currTrans->lock);
  release(&log[dev].lock);
}

// prints the current transaction
void
print_log_header(int dev)
{
  // int currTransIdx = log[dev].transcount % 2;
  // struct transaction *currTrans = &log[dev].transactions[currTransIdx];
  int i;
  printf("[");
  for(i = 1; i < LOGSIZE; i++) {
    printf("%d, ", log[dev].lh.block[i]);
  }
  printf("]\n");
}
