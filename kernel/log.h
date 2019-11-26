#define DESCRIPTORMAGIC 0xb32803b

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

struct descriptorBlock {
  uint magic;
  int seqNum;
  int block[TRANSSIZE];
};

struct commitBlock {
  uint magic;
  int seqNum;
};

// currently we just have 2 transactions, find the current one 
// by doing modulo 2 on transcount, we just keep incrementing the 
// transcount
struct log {
  struct spinlock lock;
  int start;
  int size;
  int dev;
  struct transaction transactions[MAXTRANS];
  int transcount;
  int currSeqNum;
  struct logheader lh;
};

