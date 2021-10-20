CC=gcc
CFLAGS=-lpthread -lrt

%.o: libs/%.c %.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

simulator: simulator.o libs/queue.o
	$(CC) -g -o simulator.out simulator.o libs/queue.o $(CFLAGS)

manager: manager.o
	$(CC) -g -o manager.out manager.o $(CFLAGS)

tests: tests.o libs/map.o libs/queue.o 
	$(CC) -g -o tests.out tests.c libs/map.o libs/queue.o $(CFLAGS)

.PHONY: clean

clean:
	rm *.o
	rm libs/*.o
