#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include "libs/types.h"
#include "libs/billing.h"
#include "libs/map.h"
#include "libs/leveldata.h"

// max number of entrances, exits and levels is 5, 
// always just have 5 elements in this array
// as 5 ints is small
int thread_number[5] = {1, 2, 3, 4, 5};

// no need to lock this because after the values are written into the list 
// in main they won't change and there will be only reads
Map allow_list;

// gate opening times
timespec gate_times[ENTRANCE];

// level_m protexts level_d and free_spots
pthread_mutex_t level_m;
level_data_t leveld;
size_t free_spots;

pthread_mutex_t billing_m;
Billing billing;

volatile void * shm; 

void* entrance_thr(void* arg){
    int tn = *(int*)arg;    

    // signal the simluator thread that we are ready, then wait on the entrance lpr 
    // condition variable
    // LOCK LPR
    pthread_mutex_lock(&ENTRANCE_LPR(tn,shm).m); 

    // signal the simluator that we are ready
    pthread_cond_signal(&ENTRANCE_LPR(tn, shm).c);
    
    while(1){
        // UNLOCK LPR
        pthread_cond_wait(&ENTRANCE_LPR(tn, shm).c, &ENTRANCE_LPR(tn, shm).m);

        // LOCK LPR     
        pthread_mutex_lock(&ENTRANCE_LPR(tn,shm).m); 

        char * rego = calloc(7);
        assert(rego != NULL);
            
        // read rego 
        memcpy(rego, &ENTRANCE_LPR(tn, shm).rego, 6);
        rego[6] = '\0';
        
        // check if the rego is on the allow list 
        if(!exists(allow_list, rego)){
            // rego not allowed in, show 'X' on the sign
            pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm).m);
            ENTRANCE_SIGN(tn, shm).display = "X";
            pthread_cond_signal(&ENTRANCE_SIGN(tn, shm).c;
            pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm).m);

            // everything but LPR is unlocked and LPR is locked
            // ready to wait on the signal in the next iteration
            // of the loop
            continue;
        }

        // LOCK LEVEL DATA
        pthread_mutex_lock(&level_m);     
        assert(free_spots >= 0);
        // no spaces in the car park show 'X' on the sign
        if(free_spots == 0){
            // UNLOCK LEVEL DATA
            pthread_mutex_unlock(&level_m);

            pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm).m);
            ENTRANCE_SIGN(tn, shm).display = "X";
            pthread_cond_signal(&ENTRANCE_SIGN(tn, shm).c;
            pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm).m);

            // everything but LPR is unlocked and LPR is locked
            // ready to wait on the signal in the next iteration
            // of the loop
            continue;
        }
        size_t level = get_available_level(&level_d); 
        assert(level != 0);
        insert_in_level(&level_d, level, rego, false);
        // UNLOCK LEVEL DATA
        pthread_mutex_unlock(&level_m);
        
        // LOCK BILLING DATA
        pthread_mutex_lock(&billing_m);        
        insert_rego(&billing, rego);  
        // UNLOCK BILLING DATA
        pthread_mutex_unlock(&billing_m);

        // The car has been assigned a level now show that 
        // level on the sign and signal the car to read the 
        // sign
        
        // ascii encode the level number
        char level_char = 48 + level; 
        pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm).m);
        ENTRANCE_SIGN(tn, shm).display = level_char;
        pthread_cond_signal(&ENTRANCE_SIGN(tn, shm).c;
        pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm).m);

        // LOCK BOOM GATE
        pthread_mutex_lock(&ENTRANCE_BOOM(tn, shm).m);
        
        if(ENTRANCE_BOOM(tn, shm).state == "C"){
            // closed set to raising.
            ENTRANCE_BOOM(tn, shm).state = "R";     
            pthread_cond_signal(&ENTRANCE_BOOM(tn, shm).c);
            pthread_cond_wait(&ENTRANCE_BOOM(tn, shm).c, &ENTRANCE_BOOM(tn, shm).m);
        } 
        else{
            // if gate has been open for more than 20 ms
            timespec now;
            clock_gettime(CLOCK_MONOTONIC, now); 
             
        }
        
    }
}

void* exit_thr(void* arg){
    int tn = *(int*)arg;    
}

void* level_thr(void*arg){
    int tn = *(int*)arg;    
}

int main(){
    /// setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR, 0);
    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }

    free_spots = LEVELS * LEVEL_CAPACITY; 

    /// memory map the shared memory
	shm = mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // set up the allowed license plate map here

    // setup the billing and billing mutex
    init_billing(&billing);
    pthread_mutex_init(billing_m, NULL);

    // setup the level data and level data mutex
    //init_level_data(&level_data);

    pthread_t * entrance_threads = malloc(sizeof(pthread_t) * ENTRANCES);
    pthread_t * exit_threads = malloc(sizeof(pthread_t) * EXITS);
    pthread_t * level_threads = malloc(sizeof(pthread_t) * LEVELS); 

    // start entry threads
    for(int i = 0; i < ENTRANCES; i++){
        pthread_create(entrance_threads+i, NULL, &entrance_thr, &thread_number[i]);         
    }

    // start exit threads
    for(int i = 0; i < EXITS; i++){
        pthread_create(exit_threads+i, NULL, &exit_thr, &thread_number[i]);         
    }

    // start level threads
    for(int i = 0; i < LEVELS; i++){
        pthread_create(exit_threads+i, NULL, &level_thr, &thread_number[i]);         
    }
    
    /// wait for all threads to exit
    void*retval;
    for(int i = 0; i < ENTRANCES; i++){
        pthread_join(*(entrance_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }

    // wait on threads to exit
    // close shared memory
    return 0;
}
