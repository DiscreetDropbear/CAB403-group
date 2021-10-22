#ifndef TIMING_H
#define TIMEING_H
#include <time.h>

// get the difference between now and the given time in milliseconds
// and stores it in size_t* milli
// if clock_gettime() returns a non zero value that is returned and
// milli should not be read 
int time_diff(timespec before, size_t* milli);

#endif
