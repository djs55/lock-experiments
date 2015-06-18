CFLAGS+=-g -Wall

all: main

main: main.o lock.o
	$(CC) main.o lock.o -o main -lpthread

.PHONY: clean
clean:
	rm *.o main
