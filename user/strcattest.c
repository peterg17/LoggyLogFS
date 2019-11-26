#include "kernel/types.h"
#include "user/user.h"
// #include "kernel/spinlock.h"
// #include "kernel/proc.h"


int main(int argc, char *argv[]) {

  if (argc > 3) {
    printf("sleep: error too many arguments\n");
    exit(1);
  } else if (argc <= 2) {
    printf("sleep: error not enough args provided\n");
    exit(1);
  } else {
    char* strA = argv[1];
    char* strB = argv[2];
    // int numTicks = atoi(charTicks);
    // printf("numTicks:%d\n", numTicks);
    printf("first string was: %s\n", strA);
    strcat(strA, strB);
    printf("first string is now: %s\n", strA);
    exit(0);
  }
}