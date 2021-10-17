CC=gcc
CFLAGS=-lpthread -lrt

%.o: libs/%.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

simulator: simulator.o libs/queue.o
	$(CC) -g -o simulator simulator.o libs/queue.o $(CFLAGS)

manager: manager.o
	$(CC) -g -o manager manager.o $(CFLAGS)

.PHONY: clean

clean:
	rm *.o
	rm libs/*.o
