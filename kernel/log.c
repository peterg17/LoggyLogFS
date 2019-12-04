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

// TODO: figure out when/how to reset 
// the logheader when it becomes full


struct log log[NDISK];

// in-memory copies of logged blocks
struct memlog {
  // struct spinlock lock;
  struct buf buf[LOGSIZE];
  struct logheader lh;
};

struct memlog mlog[NDISK];

static void recover_from_log(int);
static void commit(int, void *);
void print_log_header(int);

// TODO: can we write something in c that will take in a printf string
//       and arbitrary number of arguments, and check a boolean that only
//       calls to printf if we set the debug variable to 1?

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
  for(int i = 0; i < NTRANS; i++) {
    initlock(&log[dev].transactions[i].lock, "txn");
  }
  log[dev].start = sb->logstart;
  log[dev].size = sb->nlog;
  log[dev].dev = dev;
  recover_from_log(dev);
}

// Copy committed blocks from log to their home location
static void
install_trans(int dev)
{
  // printf("installing blocks to their home locations...\n");
  int tail;

  // I think the code is the same for installing as it was before
  // at least in our simple scheme where we use one logheader and clobber
  // it each time, rather than a circular array of descriptor ... commit block pairs
  for (tail = 0; tail < log[dev].lh.n; tail++) {

    // read from in-memory log block rather than from on disk log
    struct buf* mlbuf = &mlog[dev].buf[tail];
    //struct buf *lbuf = bread(dev, log[dev].start+tail+1); // log block
    struct buf *dbuf = bread(dev, log[dev].lh.block[tail]); // destination block
    memmove(dbuf->data, mlbuf->data, BSIZE);  // move log block to its destination
    bwrite(dbuf);
    bunpin(dbuf);
    //brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(int dev)
{
  struct buf *buf = bread(dev, log[dev].start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log[dev].lh.n = lh->n;
  for (i = 0; i < log[dev].lh.n; i++) {
    log[dev].lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(int dev, void *logheader)
{
  struct logheader *lh = (struct logheader *) logheader;
  struct buf *buf = bread(dev, log[dev].start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = lh->n;
  for(i = 0; i < lh->n; i++) {
    hb->block[i] = lh->block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(int dev)
{
  read_head(dev);
  install_trans(dev); // if committed, copy from log to disk
  log[dev].lh.n = 0;
  write_head(dev, &log[dev].lh);

}

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
      printf("sleeping on currently committing transaction\n");
      sleep(currTrans, &currTrans->lock);
    } else if(currTrans->blocksWritten + ((currTrans->outstanding + 1)*MAXOPBLOCKS) > TRANSSIZE) {
      // Explanation: essentially what we are doing here is planning for the worst 
      // ie-> pretending that every syscall yet to complete will take MAXOPBLOCKS when they likely wont
      // we wakeup this transaction when each outstanding syscall finishes to check what actually happened
      // TODO: answer if the transaction counter would ever change between a begin_op and an end_op?
      printf("sleeping on full log!\n");
      sleep(currTrans, &currTrans->lock);
    } else {
      currTrans->outstanding += 1;
      release(&currTrans->lock);
      break;
    }
  }
}

// returns 1 if the on-disk log is >50% full, 0 otherwise
int
is_ondisklog_full(int dev)
{
  int isLogFull;
  int maxCapacity = LOGSIZE / 2;
  acquire(&log[dev].lock);
  isLogFull = log[dev].lh.n > maxCapacity;
  release(&log[dev].lock);
  return isLogFull;
}


void
sync_helper(int dev) {
  // why do we call commit w/o holding locks?
  // can we still hold locks because we aren't sleeping during commit?
  
  // 1. block new syscalls somehow? by sleeping on curr trans
  // sleep(currtrans, &currtrans->lock);

  // if we are trying to commit but there isn't more space in log,
  // we need to sleep on the current transaction so we don't overwrite
  // the on-disk log
  int currTxnIndex = log[dev].transcount % 2; 
  struct transaction *currtrans = &log[dev].transactions[currTxnIndex];

  // switch to new transactions
  acquire(&log[dev].lock);
  log[dev].transcount += 1;
  release(&log[dev].lock);
  

  commit(dev, &log[dev].lh);

  if(is_ondisklog_full(dev)) {
    install_trans(dev);
    log[dev].lh.n = 0;
    write_head(dev, &log[dev].lh);
  }

  acquire(&currtrans->lock);
  // TODO: when we are incrementing the transaction counter, make sure that there are no
  // outstanding syscalls, because that could lead to a hairy situation
  currtrans->committing = 0;
  currtrans->blocksWritten = 0;
  // we have no wakeup call here because we don't sleep when committing
  // TODO: possible wakeup call on each transaction lock
  // because we can't possibly write to a transaction while its committing
  wakeup(currtrans);
  release(&currtrans->lock);
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
  int isTxnFull;
  struct transaction *currtrans;
  // struct logheader snapshotLH;

  int do_commit = 0;

  acquire(&log[dev].lock);
  // TODO: replace this 2 aka magic number with the number of transactions variable
  currTxnIndex = log[dev].transcount % 2; 
  currtrans = &log[dev].transactions[currTxnIndex];
  acquire(&currtrans->lock);
  currtrans->outstanding -= 1;
  // printf("number of outstanding syscalls in txn is: %d\n", currtrans->outstanding);
  isTxnFull = (currtrans->blocksWritten + MAXOPBLOCKS) > TRANSSIZE;

  if(currtrans->outstanding < 0) {
    panic("outstanding syscalls in txn is < 0!\n");
  } else if (currtrans->outstanding == 0 && isTxnFull) {
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
  // memmove(&snapshotLH, &log[dev].lh, sizeof(log[dev].lh));

  release(&currtrans->lock);  
  release(&log[dev].lock);
  
  if(do_commit){
    sync_helper(dev);
  }
}

// Copy modified blocks from cache to log.
static void
write_log(int dev, void *logheader)
{
  struct logheader *lh = (struct logheader *) logheader;
  int tail;
  for(tail = 0; tail < lh->n; tail++) {
    struct buf *to = bread(dev, log[dev].start+tail+1); // log block
    struct buf *from = bread(dev, lh->block[tail]); // cached block
    struct buf* mto = &mlog[dev].buf[tail];

    memmove(to->data, from->data, BSIZE);
    memmove(mto->data, from->data, BSIZE);

    bwrite(to); // writing the log block
    //bunpin(from);  // unpin during commit
    brelse(from);
    brelse(to);
  }
}

static void
commit(int dev, void *logheader)
{
  printf("calling commit\n");
  struct logheader *lh = (struct logheader *) logheader;
  if(lh->n > 0) {
    write_log(dev, logheader);     // Write modified blocks from cache to log
    write_head(dev, logheader);    // Write header to disk -- the real commit

    // don't install transaction yet, defer it until on-disk log is full enough
  }
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
  // printf("in log write function\n");

  int i;
  int dev = b->dev;
  int currTransIdx = log[dev].transcount % 2;
  struct transaction *currTrans = &log[dev].transactions[currTransIdx];
  // print_log_header(dev);

  if (log[dev].lh.n >= LOGSIZE || log[dev].lh.n >= log[dev].size - 1)
    panic("log is full");
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
  // printf("blocks written to transaction: %d\n", currTrans->blocksWritten);
  // printf("current logheader sum: %d\n", log[dev].lh.n);
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
