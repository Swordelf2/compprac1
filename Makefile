all: compprac1

CC=gcc
LD=gcc
CFLAGS=-g -std=gnu++17 -Wall -Wextra -Iqbe/include
LDFLAGS=-Lqbe/lib -lqbe

main.o: main.cpp
	$(CC) -c $(CFLAGS) $(LDFLAGS) $< -o $@

compprac1: main.o
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f main.o compprac1
