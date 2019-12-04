#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user.h"

void create_small_files(int n) 
{
  int fd;
  int cycles;

  cycles = uptime();

  for (int i = 0; i < n; i++) {

    // printf("creating a file\n");

    int fileNameLength;
    int recycledI;
    fileNameLength = (i / 40) + 1; // cycle through ascii, each time increase file length by 1
    recycledI = i % 40;

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

    // if(unlink(fname) < 0){
    //   printf("unlink failed small file:  %s\n", fname);
    //   exit(1);
    // }
  }

  cycles = uptime() - cycles;
  printf("cycles elapsed during smallfiles benchmark: %d\n", cycles);
}

int main(int argc, char *argv[])
{
  int nfiles, i;
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

  printf("creating %d files...\n", nfiles);

  create_small_files(nfiles);

  exit(0);
}
