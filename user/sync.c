#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "user.h"


int main(int argc, char *argv[])
{
  if(argc != 1) {
    printf("No arguments passed into sync\n");
    exit(1);
  }
  
  sync();

  exit(0);
}