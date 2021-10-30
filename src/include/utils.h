#ifndef TIMING_H
#define TIMING_H
#include <stdbool.h>
#include <time.h>

// create the mutex's and condition variables and set them to PTHREAD_PROCESS_SHARED 
// for each lpr, boom and sign in entrances, exits and levels
void init_shared_mem(volatile void * shm);

int future_time(struct timespec* out, int milli);
bool has_past(struct timespec time);
int time_diff(struct timespec before, unsigned long * milli);

int load_regos( char** * regos, int* num_regos);


#endif
