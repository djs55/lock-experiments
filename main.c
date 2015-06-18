#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <linux/limits.h> /* for PATH_MAX */
#include <errno.h>
#include "lock.h"

int verbose = 0;

/* Called to reset the Xen watchdog timer */
void tickle_watchdog() {

}

void main_loop(const char *host_lock_path,
               const char *master_lock_path,
               const char *fence_directory,
               char **other_lock_paths,
               int n_other_lock_paths) {
  struct lock host_lock, master_lock;
  struct lock *other_locks;
  int *host_has_self_fenced;
  enum state state;
  int i;

  lock_init(&host_lock, host_lock_path, fence_directory);
  lock_init(&master_lock, master_lock_path, fence_directory);

  other_locks = (struct lock*) malloc(n_other_lock_paths * sizeof (struct lock));
  host_has_self_fenced = (int *) malloc(n_other_lock_paths * sizeof(int));
  if (!other_locks || !host_has_self_fenced) abort ();
  for (i = 0; i < n_other_lock_paths; i++) {
    lock_init(other_locks + i, *(other_lock_paths + i), fence_directory);
    *(host_has_self_fenced + i) = 0;
  }

  while(1) {
    sleep(1);

    tickle_watchdog ();

    state = lock_acquire(&host_lock);
    if (state == ACQUIRED) {
      printf("I have joined the cluster and can safely start VMs.\n");
      fflush(stdout);
    }
    if (state == LOST) {
      printf("I have lost my host lock, I must self-fence.\n");
      exit(1); /* let the watchdog kill us */
    }

    state = lock_acquire(&master_lock);
    if (state == ACQUIRED) {
      printf("I have taken the master role.\n");
      fflush(stdout);
    }
    if (state == LOST) {
      printf("I have lost the master lock, I must self-fence.\n");
      exit(1); /* let the watchdog kill us */
    }

    if (master_lock.acquired) {
      for (i = 0; i < n_other_lock_paths; i++) {
        if (*(host_has_self_fenced + i)) {
          /* We have already restarted this host's VMs after it self-fenced.
             We wait for it to come back online. */
          if (is_lock_held(other_locks + i)) {
            printf("Another host has re-locked %s.\n", (other_locks + i)->filename);
            fflush(stdout);
            *(host_has_self_fenced + i) = 0;
            /* We will now start competing with this host for its lock. */
          }
        } else {
          /* We constantly try to acquire another host's lock to sieze
             its resources and restart its VMs. */
          state = lock_acquire(other_locks + i);
          if (state == ACQUIRED) {
            printf("I have sized the lock %s: It must have self-fenced. I should restart VMs.\n", (other_locks + i)->filename);
            fflush(stdout);
            *(host_has_self_fenced + i) = 1;
            lock_release(other_locks + i);
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  char *uuid; /* This host's uuid */
  char *fence_directory; /* Touch a file here to self-fence */
  int i, c, digit_optind = 0;
  char uuid_lock_path[PATH_MAX];
  char **other_lock_files;

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"uuid",    required_argument, 0,  'u' },
      {"fencedir",required_argument, 0,  'f' },
      {"verbose", no_argument,       0,  'v' },
      {0,         0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "u:f:v", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 'u':
        uuid = strdup(optarg);
        break;
      case 'f':
        fence_directory = strdup(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        fprintf(stderr, "unexpected return from getopt: %d\n", c);
    }
  }

  if (!uuid) {
    fprintf(stderr, "Please supply a host uuid with --uuid <uuid>\n");
    exit(1);
  }
  if (!fence_directory) {
    fprintf(stderr, "Please supply a self-fence directory with --fencedir <dir>\n");
    exit(1);
  }

  if ((mkdir(".ha", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .ha");
  }
  if ((mkdir(".ha/host", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .ha/host");
  }

  snprintf(uuid_lock_path, sizeof(uuid_lock_path), ".ha/host/%s.lock", uuid);
  other_lock_files = (char **)malloc(sizeof(char *) * (argc - optind));
  if (!other_lock_files)
    abort();  
  for (i = 0; i < (argc - optind); i++) {
    int bytes = snprintf(NULL, 0, ".ha/host/%s.lock", argv[optind + i]);
    char *path = malloc(bytes);
    snprintf(path, bytes+1, ".ha/host/%s.lock", argv[optind + i]);
    *(other_lock_files + i) = path;
  }

  main_loop(uuid_lock_path, ".ha/master.lock", fence_directory, other_lock_files, argc - optind);
}
