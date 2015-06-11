
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

extern void lock_init(struct lock *lock, const char *filename);

/* Attempt to acquire and hold a lock */
extern enum state lock_acquire(struct lock *lock);

