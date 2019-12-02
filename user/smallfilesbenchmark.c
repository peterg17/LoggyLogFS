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

// returns the number of cycles that creating n files took
int create_small_files(int n) 
{
  int fd;
  int cycles;

  cycles = uptime();

  for (int i = 0; i < n; i++) {
    int fileNameLength;
    int recycledI;
    fileNameLength = (i / 26) + 1; // cycle through the alphabet, each time increase file length by 1
    recycledI = i % 26;

    char fname[fileNameLength];
    for (int j = 0; j < fileNameLength; j++) {
      fname[j] = (char) (recycledI + 65);
    }

    fd = open(fname, O_CREATE|O_RDWR);
    if(fd < 0){
      printf("Error: create small failed!\n");
      printf("length of fname: %d\n", sizeof(fname)/sizeof(char));
      exit(1);
    }
    write(fd, "aaa", 3);
    close(fd);

    if(unlink(fname) < 0){
      printf("unlink failed small file:  %s\n", fname);
      exit(1);
    }
  }

  cycles = uptime() - cycles;
  return cycles;
}

int main(int argc, char *argv[])
{
  int nfiles, tests, i, avgClockCycles;
  int clockCycles = 0;

  if(argc != 1) {
    printf("No argument to smallfiles benchmark\n");
    exit(1);
  } 

  nfiles = 1000;
  tests = 100;

  for (i = 0; i < tests; i++) {
    printf("creating %d files...\n", nfiles);
    clockCycles += create_small_files(nfiles);
  }

  avgClockCycles = clockCycles / tests;
  printf("average clock cycles for creating %d files is --> %d\n", nfiles, avgClockCycles);
  
  return 0;

}