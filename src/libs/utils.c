#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
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
    (void) pthread_condattr_setclock(&pcond_attr, CLOCK_REALTIME);
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
        memcpy(&ENTRANCE_LPR(i, shm)->rego, "------", 6);
        
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
        memcpy(&EXIT_LPR(i, shm)->rego, "------", 6);
        
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
        memcpy(&LEVEL_LPR(i, shm)->rego, "------", 6);

        *LEVEL_ALARM(i, shm) = 0;
        *LEVEL_TEMP(i, shm) = 0;
    }
}

// adds number of milliseconds onto the current time and returns 
// it in timespec
int future_time(struct timespec* out, int milli){
    int res = clock_gettime(CLOCK_REALTIME, out);
    const int billion = 1000000000;

    milli = milli * SLEEP_SCALE;

    time_t secs = milli / 1000;
    milli = milli % 1000;
    unsigned long nsec = 1000000 * milli;

    // tv_nsec needs to be between 1 and a billion
    // if its over then add the number of billions
    // its over as seconds and set tv_nsec to the remainder

    out->tv_sec += secs;
    out->tv_nsec = out->tv_nsec % billion;
}

// compares the current time with another specified time and returns 
// true if the current time is passed the othe time
bool has_past(struct timespec time){
    struct timespec now;
    int res = clock_gettime(CLOCK_REALTIME, &now);
    if(res != 0){
        fprintf(stderr, "error getting time: errno %d\n", errno); 
        abort();
    }
    
    if(time.tv_sec < now.tv_sec){
        return true;
    }
    else if(time.tv_sec == now.tv_sec && time.tv_nsec <= now.tv_nsec){
        return true;
    }
    else{
        return false;
    }
}

// TODO: take into account the sleep modifier here so in testing
// this will still work
int time_diff(struct timespec before, unsigned long * milli){
    assert(milli != NULL);
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);

    if(res != 0){
        fprintf(stderr, "error getting time: errno %d\n", errno); 
        abort();
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


// function to retrive number of lines in plates txt file
// will store the number of lines in variable n
// 
int load_regos( char*** regos, int* num_regos){
    //have to find number of lines in file
    FILE *fp;
    size_t n = 0; 
    char c;  // To store a character read from file
  
    // Open the file
    fp = fopen("plates.txt", "r");
  
    // Check if file exists
    if (fp == NULL){
        printf("Could not open file");
        return 0;
    }
  
    // find the number of lines in the file
    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n') {
            n = n + 1;
        }
    }
    // set the cursor back to the begining of the file
    fseek(fp, 0, SEEK_SET);
    
    // set the external value of num_regos so
    // the caller knows how many regos there are
    *num_regos = n;
     
    // allocate the required memory to read in the regos
    char ** values = malloc(sizeof(char*) * n); 
    assert(values != NULL);

    // allocate the required memory for each of the regos
    for(int i = 0; i<n; i++){
        values[i] = calloc(7, sizeof(char));
        assert(values[i] != NULL); 
    }

    // read the rego for each line 
    for(int a = 0; a<n; a++){
        int ret = fscanf(fp, "%c%c%c%c%c%c\n", &values[a][0],&values[a][1],&values[a][2],&values[a][3],&values[a][4],&values[a][5]);
        if(ret != 6){
            fprintf(stderr, "there was an error reading the plates.txt file");
            exit(-1);
        }
    }

    // set the memory address of the regos to the callers
    // variable
    *regos = values;

    fclose(fp);
    return 0;
}
