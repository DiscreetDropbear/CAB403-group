#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "../include/utils.h"

// TODO: take into account the sleep modifier here so in testing
// this will still work
int time_diff(struct timespec before, unsigned long * milli){
    assert(milli != NULL);
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    
    // failure
    if(res != 0){
        return res;
    }

    //success
    assert(before.tv_sec <= now.tv_sec);
    unsigned long diff_milli = (now.tv_sec - before.tv_sec) * 1000; 

    if(now.tv_nsec > before.tv_nsec){
        diff_milli += (unsigned long)((now.tv_nsec - before.tv_nsec)/1000000);    
    }
    else{
        diff_milli += (unsigned long)((before.tv_nsec - now.tv_nsec)/1000000);    
    }
    
    *milli = diff_milli;

    return 0;
}
