all: main inotify

main: main.o lock.o

inotify: inotify.o
