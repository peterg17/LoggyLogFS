#include "spinlock.h"
#include "param.h"
#include "types.h"

#define DESCRIPTORMAGIC 0xb32803b

struct transaction {
  struct spinlock lock;
  int seqNum; //sequence # of transaction (transaction id)
  int syscallct;
  int block[TRANSIZE];
};

struct logsuperblock {
  int offset;
  int startSeqNum;
};

struct descriptorBlock {
  uint magic;
  int seqNum;
  int block[TRANSIZE];
};

struct commitBlock {
  uint magic;
  int seqNum;
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int committing;
  int dev;
  struct transaction transactions[MAXTRANS];
  struct transaction* cur_trans;
};

