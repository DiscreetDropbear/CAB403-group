#ifndef TIMING_H
#define TIMING_H
#include <time.h>

// create the mutex's and condition variables and set them to PTHREAD_PROCESS_SHARED 
// for each lpr, boom and sign in entrances, exits and levels
void init_shared_mem(volatile void * shm);

// get the difference between now and the given time in milliseconds
// and stores it in size_t* milli
// if clock_gettime() returns a non zero value that is returned and
// milli should not be read 
int time_diff(struct timespec before, size_t* milli);



#endif
