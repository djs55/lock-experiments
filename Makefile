CFLAGS+=-g

all: main inotify

main: main.o lock.o
	$(CC) main.o lock.o -o main -lpthread


inotify: inotify.o

.PHONY: clean
clean:
	rm *.o inotify main
