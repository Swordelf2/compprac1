all: compprac1

CC=g++
LD=g++
CFLAGS=-g -std=gnu++17 -Wall -Wextra
LDFLAGS=-Lqbe/lib

main.o: main.cpp
	$(CC) -c $(CFLAGS) $(LDFLAGS) $< -o $@

compprac1: main.o
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f main.o compprac1
