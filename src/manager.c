#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include "include/macros.h"
#include "include/types.h"
#include "include/billing.h"
#include "include/map.h"
#include "include/leveldata.h"
#include "include/utils.h"
#include "include/manager_funcs.h"

// TODO: change thread nums to start at 0
//       and change macros to work assuming thead nums start at 0
int main(){

    /// initialise all of the variables that the threads will use
    // no need to lock this because after the values are written into the list 
    // in main they won't change and there will be only reads
    Map allow_list;
    // level_m protexts level_d and free_spots
    pthread_mutex_t level_m;
    level_data_t level_d;
    size_t free_spots;
    pthread_mutex_t billing_m;
    billing_t billing;
    volatile void * shm; 

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
    pthread_mutex_init(&billing_m, NULL);

    //TODO: setup the level data and level data mutex
    //init_level_data(&level_data);
    
    //TODO: setup allow_list

    pthread_t * entrance_threads = malloc(sizeof(pthread_t) * ENTRANCES);
    pthread_t * exit_threads = malloc(sizeof(pthread_t) * EXITS);
    pthread_t * level_threads = malloc(sizeof(pthread_t) * LEVELS); 

    entrance_args_t entrance_args[5];
    level_args_t level_args[5];
    exit_args_t exit_args[5];

    // start entry threads
    for(int i = 0; i < ENTRANCES; i++){

        entrance_args[i].thread_num = i+1; 
        entrance_args[i].shared_mem = shm; 
        entrance_args[i].allow_list = &allow_list; 
        entrance_args[i].billing_m = &billing_m;
        entrance_args[i].billing = &billing;
        entrance_args[i].level_m = &level_m;
        entrance_args[i].level_d = &level_d;
        entrance_args[i].free_spots = &free_spots;
        pthread_create(entrance_threads+i, NULL, &entrance_thread, &entrance_args[i]);         
    }

    // start exit threads
    for(int i = 0; i < EXITS; i++){

        exit_args[i].thread_num = i+1; 
        exit_args[i].shared_mem = shm; 
        exit_args[i].billing_m = &billing_m;
        exit_args[i].billing = &billing;
        exit_args[i].level_m = &level_m;
        exit_args[i].level_d = &level_d;
        exit_args[i].free_spots = &free_spots;
        pthread_create(exit_threads+i, NULL, &exit_thr, &exit_args[i]);         
    }

    // start level threads
    for(int i = 0; i < LEVELS; i++){
        level_args[i].thread_num = i+1;
        level_args[i].shared_mem = shm;
        level_args[i].level_m = &level_m;
        level_args[i].level_d = &level_d;
        pthread_create(exit_threads+i, NULL, &level_thr, &level_args[i]);         
    }
    
    /// wait for all threads to exit
    void* retval;
    for(int i = 0; i < ENTRANCES; i++){
        pthread_join(*(entrance_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }

    //TODO: close shared memory
    
    return 0;
}
