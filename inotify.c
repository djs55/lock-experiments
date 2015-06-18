#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

void main(int argc, char *argv[]){
  int fd, watch, n;
  struct inotify_event *event;
  char buf[sizeof(struct inotify_event) + NAME_MAX + 1];

  if ((mkdir(".ha", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .ha");
  }
  if ((mkdir(".ha/always-empty-directory", 0755) == -1) && (errno != EEXIST)) {
    perror("Failed to mkdir .ha/always-empty-directory");
  }

  if ((fd = inotify_init()) == -1) {
    perror("Failed to initialise inotify");
    exit (1);
  }
  if ((watch = inotify_add_watch(fd, ".ha/always-empty-directory", IN_CREATE)) == -1) {
    perror("Failed to add inotify watch");
    exit (1);
  }
  while (1) {
    if ((n = read(fd, buf, sizeof(buf))) <= 0) {
      fprintf(stderr, "Failed to read from inotify, self-fencing\n");
      exit (1);
    }
    event = (struct inotify_event *) buf;
    fprintf(stderr, "Received an event from inotify, self-fencing\n");    
    exit (1);
 }
}
