#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

/* If we lose the lock, we guarantee to self-fence after this time. */
const int self_fence_interval = 5;

void tickle_watchdog() {
  fprintf(stderr, "I am holding the lock; resetting watchdog timer; will self-fence in %d seconds.\n", self_fence_interval);
}

int lock(const char *filename) {
  struct flock l;
  int ret, fd, nattempts;

  l.l_type = F_WRLCK;
  l.l_start = 0; /* from the start... */
  l.l_len = 0;   /* ... to the end */

  if ((fd = open(filename, O_RDWR | O_CREAT, 0644)) == -1) {
    perror(filename);
    _exit(1);
  }
acquiring:
  nattempts = self_fence_interval * 2; /* NB: safety margin */
  while (nattempts-- > 0) {
    sleep(1);
    if (fcntl(fd, F_SETLK, &l) == -1) {
      fprintf(stderr, "Failed to acquire the lock on %s\n", filename);
      fflush(stderr);
      goto acquiring;
    } else {
      fprintf(stderr, "If everything is ok, I will acquire the lock in %d seconds\n", nattempts + 1);
      fflush(stderr);
    }
  }
holding:
  while (1) {
    sleep(1);
    if (fcntl(fd, F_SETLK, &l) == -1) {
      fprintf(stderr, "Lost the lock on %s\n", filename);
      _exit(1);
    }
    tickle_watchdog ();
  }
}

int main(int argc, char *argv[]){
  if (argc != 2) {
    fprintf(stderr, "Usage:\n%s <lock filename>\n\tAcquire and hold the named lock\n", argv[0]);
    _exit(1);
  }
  lock(argv[1]);
  return 0;
}
