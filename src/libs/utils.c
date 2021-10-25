#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include "../include/macros.h"
#include "../include/utils.h"

// create the mutex's and condition variables and set them to PTHREAD_PROCESS_SHARED 
// for each lpr, boom and sign in entrances, exits and levels
void init_shared_mem(volatile void * shm){
    pthread_mutexattr_t pmut_attr;
    pthread_condattr_t pcond_attr;

    (void) pthread_mutexattr_init(&pmut_attr);
    (void) pthread_mutexattr_setpshared(&pmut_attr,
        PTHREAD_PROCESS_SHARED);
    (void) pthread_condattr_init(&pcond_attr);
    (void) pthread_condattr_setpshared(&pcond_attr,
        PTHREAD_PROCESS_SHARED);
    /// ENTRANCES
    // initates mutex's and condition variables for the entrances 
    for(int i = 1; i <= ENTRANCES; i++){
        /// LPR
        // setup mutex
        pthread_mutex_init(&ENTRANCE_LPR(i, shm)->m, &pmut_attr);

        // setup condition var
        pthread_cond_init(&ENTRANCE_LPR(i, shm)->c, &pcond_attr);
        
        /// BOOM
        // setup mutex
        pthread_mutex_init(&ENTRANCE_BOOM(i, shm)->m, &pmut_attr);

        // setup condition var
        pthread_cond_init(&ENTRANCE_BOOM(i, shm)->c, &pcond_attr);
        ENTRANCE_BOOM(i, shm)->state = 'C';
         
        /// SIGN 
        // setup mutex
        pthread_mutex_init(&ENTRANCE_SIGN(i, shm)->m, &pmut_attr);
        // setup condition var
        pthread_cond_init(&ENTRANCE_SIGN(i, shm)->c, &pcond_attr);
    }

    /// EXITS
    // initates mutex's and condition variables for the exits 
    for(int i = 1; i <= EXITS; i++){
        /// LPR
        // setup mutex
        pthread_mutex_init(&EXIT_LPR(i, shm)->m, &pmut_attr);
        // setup condition var
        pthread_cond_init(&EXIT_LPR(i, shm)->c, &pcond_attr);
        
        /// BOOM
        // setup mutex
        pthread_mutex_init(&EXIT_BOOM(i, shm)->m, &pmut_attr);
        // setup condition var
        pthread_cond_init(&EXIT_BOOM(i, shm)->c, &pcond_attr);
        EXIT_BOOM(i, shm)->state = 'C';
    }

    /// LEVELS
    // initates mutex's and condition variables for the levels
    for(int i = 1; i <= LEVELS; i++){
        /// LPR
        // setup mutex
        pthread_mutex_init(&LEVEL_LPR(i, shm)->m, &pmut_attr);
        // setup condition var
        pthread_cond_init(&LEVEL_LPR(i, shm)->c, &pcond_attr);
    }
}



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
