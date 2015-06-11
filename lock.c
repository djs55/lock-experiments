#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

/* If we lose the lock, we guarantee to self-fence after this time. */
const int self_fence_interval = 5;

struct lock {
  const char *filename;
  int acquired;
  int fd;
  int nattempts_remaining; /* successful lock attempts remaining before
                              we enter the 'acquired' state */
};

void lock_init(struct lock *l, const char *filename) {
  l->filename = filename;
  l->acquired = 0;
  if ((l->fd = open(filename, O_RDWR | O_CREAT, 0644)) == -1) {
    perror(filename);
    _exit(1);
  }
  l->nattempts_remaining = self_fence_interval * 2; /* NB: safety margin */
}

void lock_release(struct lock *l) {
  struct flock fl;

  fl.l_type = F_UNLCK;
  fl.l_start = 0; /* from the start... */
  fl.l_len = 0;   /* ... to the end */
  fl.l_whence = SEEK_SET;

  if (fcntl(l->fd, F_SETLK, &fl) == -1) {
      fprintf(stderr, "Failed to release the lock on %s\n", l->filename);
      fflush(stderr);
  };
  l->acquired = 0;
  l->nattempts_remaining = self_fence_interval * 2;
}

int is_lock_held(struct lock *l) {
  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_start = 0; /* from the start... */
  fl.l_len = 0;   /* ... to the end */
  fl.l_whence = SEEK_SET;

  if (fcntl(l->fd, F_GETLK, &fl) == -1) {
      fprintf(stderr, "Failed query the lock on %s\n", l->filename);
      fflush(stderr);
  };
  return (fl.l_type != F_UNLCK);
}

enum state {
  FAILED,    /* someone else has the lock */
  WAITING,   /* I'm waiting for someone else to self-fence */
  ACQUIRED,  /* I've just got the lock */
  HOLDING,   /* I'm still holding the lock */
  LOST       /* I've lost the lock */
};

enum state lock_acquire(struct lock *l) {
  struct flock fl;
  int ret;

  fl.l_type = F_WRLCK;
  fl.l_start = 0; /* from the start... */
  fl.l_len = 0;   /* ... to the end */
  fl.l_whence = SEEK_SET;
  if ((!l->acquired) && (l->nattempts_remaining > 0)) {
    if (fcntl(l->fd, F_SETLK, &fl) == -1) {
      fprintf(stderr, "Failed to acquire the lock on %s\n", l->filename);
      fflush(stderr);
      return FAILED;
    } else {
      fprintf(stderr, "I am hoping to acquire %s in %d seconds\n", l->filename, l->nattempts_remaining);
      fflush(stderr);
      l->nattempts_remaining--;
      return WAITING;
    }
  };
  if (fcntl(l->fd, F_SETLK, &fl) == -1) {
    fprintf(stderr, "Lost the lock on %s\n", l->filename);
    fflush(stderr);
    return LOST;
  }
  if (!l->acquired) {
    l->acquired = 1;
    return ACQUIRED;
  }
  return HOLDING;
}
