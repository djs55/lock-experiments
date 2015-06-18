#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

/* If we lose the lock, we guarantee to self-fence after this time. */
const int self_fence_interval = 5;

struct watcher {
  const char *filename;
  int fd;
  int watch;
  pthread_t thread;
};

void *watcher_main(void *param) {
  struct watcher *w = (struct watcher *)param;
  char buf[sizeof(struct inotify_event) + NAME_MAX + 1];
  int n;

  while (1) {
    if ((n = read(w->fd, buf, sizeof(buf))) <= 0) {
      fprintf(stderr, "Failed to read from inotify, self-fencing\n");
      exit (1);
    }
    event = (struct inotify_event *) buf;
    fprintf(stderr, "Received an event from inotify, self-fencing\n");
    exit (1);
  }
}

void watcher_init(struct watcher *w, const char *filename) {
  w->filename = filename;
  if ((mkdir(w->filename, 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir fence directory");
  }
  if ((w->fd = inotify_init()) == -1) {
    perror("Failed to initialise inotify");
    exit (1);
  }
  if ((w->watch = inotify_add_watch(w->fd, w->filename, IN_CREATE)) == -1) {
    perror("Failed to add inotify watch");
    exit (1);
  }
  pthread_create(&(w->thread), NULL, watcher_main, (void *)w);
}

struct lock {
  const char *filename;
  int acquired;
  int fd;
  int nattempts_remaining; /* successful lock attempts remaining before
                              we enter the 'acquired' state */
  const char *fence_directory; /* touch a file in here to self-fence */
  struct watcher *watcher; /* when a lock is acquired we watch for errors
                              which mean we have lost it */
};

void lock_init(struct lock *l, const char *filename, const char *fence_directory) {
  l->filename = filename;
  l->fence_directory = fence_directory;
  l->acquired = 0;
  if ((l->fd = open(filename, O_RDWR | O_CREAT, 0644)) == -1) {
    perror(filename);
    _exit(1);
  }
  l->nattempts_remaining = self_fence_interval * 2; /* NB: safety margin */
  l->watcher = NULL;
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
    if ((l->watcher = (struct watcher*)malloc(sizeof(struct watcher))) == NULL) {
      perror("Failed to allocate watcher");
      exit(1);
    }
    watcher_init(l->watcher, l->fence_directory);
    return ACQUIRED;
  }
  return HOLDING;
}
