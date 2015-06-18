
struct lock {
  const char *filename;
  int acquired;
  int fd;
  int nattempts_remaining;
};

enum state {
  FAILED,    /* someone else has the lock */
  WAITING,   /* I'm waiting for someone else to self-fence */
  ACQUIRED,  /* I've just got the lock */
  HOLDING,   /* I'm still holding the lock */
  LOST       /* I've lost the lock */
};

extern void lock_init(
  struct lock *lock,
  const char *filename,       /* filename that represents the lock */
  const char *fence_directory /* touch a file in here to self-fence */
);

/* Acquire a lock. This will spawn a thread which will keep checking
   that we still hold the lock. */
extern enum state lock_acquire(struct lock *lock);

extern void lock_release(struct lock *lock);

/* Return true if someone (else) has already locked the lock */
extern int is_lock_held(struct lock *lock);
