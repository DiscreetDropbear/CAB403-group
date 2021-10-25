CC=gcc
CFLAGS=-lpthread -lrt
MANAGER_LIBS=libs/map.o libs/queue.o libs/leveldata.o libs/utils.o libs/billing.o libs/manager_funcs.o
SIMULATOR_LIBS=libs/queue.o libs/map.o
TEST_LIBS=libs/map.o libs/queue.o 

%.o: libs/%.c %.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

simulator: simulator.o $(SIMULATOR_LIBS) 
	$(CC) -g -o simulator simulator.o $(SIMULATOR_LIBS) $(CFLAGS)
	rm *.o
	rm libs/*.o

manager: manager.o manager_tests.o $(MANAGER_LIBS) 
	$(CC) -g -o manager manager.o $(MANAGER_LIBS) $(CFLAGS)
	$(CC) -g -o manager_t manager_tests.o $(MANAGER_LIBS) $(CFLAGS)
	rm *.o
	rm libs/*.o

tests: tests.o $(TEST_LIBS) 
	$(CC) -g -o tests tests.c $(TEST_LIBS) $(CFLAGS)
	rm *.o
	rm libs/*.o

.PHONY: clean

clean:
	rm *.o
	rm libs/*.o
