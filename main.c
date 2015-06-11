#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "lock.h"

int main(int argc, char *argv[]){
  if (argc != 2) {
    fprintf(stderr, "Usage:\n%s <lock filename>\n\tAcquire and hold the named lock\n", argv[0]);
    _exit(1);
  }
  lock(argv[1]);
  return 0;
}
