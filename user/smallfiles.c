#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"


void
createWriteSmallFile()
{
  int i, fd, n;
}

int main(int argc, char *argv[])
{ 
  char *n = 0;
  if(argc > 1) {
    n = argv[1];
  }

  
}