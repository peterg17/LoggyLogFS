#define DESCRIPTORMAGIC 0xb32803b
// syncs every 5 seconds
#define SYNCINTERVAL    50


void sync_helper(int);

struct transaction {
  struct spinlock lock;
  int seqNum; //sequence # of transaction (transaction id)
  int blocksWritten;
  int outstanding;
  int committing;
  int block[TRANSSIZE];
};

// struct logsuperblock {
//   int offset;
//   int startSeqNum;
// };

// for first simple version of concurrent logging, we can use one header block
struct logheader {
  int n;
  int block[LOGSIZE];
};

// currently we just have 2 transactions, find the current one 
// by doing modulo 2 on transcount, we just keep incrementing the 
// transcount
struct log {
  struct spinlock lock;
  int start;
  int size;
  int dev;
  struct transaction transactions[NTRANS];
  int transcount;
  struct logheader lh;
};

