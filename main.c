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

int main(int argc, char **argv) {
  char *uuid; /* This host's uuid */
  int c, digit_optind = 0;
  char uuid_lock_path[PATH_MAX];

  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"uuid",    required_argument, 0,  'u' },
      {"verbose", no_argument,       0,  'v' },
      {0,         0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "u:v", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
      case 'u':
        uuid = strdup(optarg);
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

  if ((mkdir(".ha", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .ha");
  }
  if ((mkdir(".ha/host", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .hg/host");
  }

  snprintf(uuid_lock_path, sizeof(uuid_lock_path), ".ha/host/%s.lock", uuid);
  lock(uuid_lock_path);
  exit(0);
}
