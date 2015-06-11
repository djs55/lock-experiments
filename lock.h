
struct lock {
  const char *filename;
  int acquired;
  int fd;
  int nattempts_remaining;
};

extern void lock_init(struct lock *lock, const char *filename);
extern void lock_poll(struct lock *lock);

