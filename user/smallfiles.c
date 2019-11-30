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

//void
//createWriteSmallFile()
//{
//  int i, fd, n;
//}

int main(int argc, char *argv[])  {

  int fd, i, nfiles;
  char *nraw = 0;

  if(argc > 1) {
    nraw = argv[1];
  } else {
    printf("Usage: smallfiles <num files>!\n");
    exit(1);
  }

  nfiles = 0;
  for (i = 0; nraw[i] != '\0'; i++)
      nfiles = nfiles * 10 + nraw[i] - '0';

  printf("creating files...\n");

  for (i = 0; i < nfiles; i++) {

    printf("creating a file\n");

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
      exit(1);
    }
    write(fd, "aaa", 3);
    close(fd);
  }

  return 0;

}
