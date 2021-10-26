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

//TODO: setup allow_list
Map allow_list;

// level data and free_spots are both protected by level_m
pthread_mutex_t level_m;
level_data_t level_d;
size_t free_spots = LEVELS * LEVEL_CAPACITY; 

volatile void * shm;

// setup the billing and billing mutex
// billing_m protects billing, total_bill and billing_fp
pthread_mutex_t billing_m;
billing_t billing;

FILE* billing_fp;  
unsigned int total_bill = 0;

int main(){
     
    /// setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR, 0);
    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }
    /// memory map the shared memory
	shm = mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        // open the biling.txt file for reading and writing in append mode 
    billing_fp = fopen("billing.txt", "a+");
    if(billing_fp == NULL){
        fprintf(stderr, "Error opening billing.txt(errno: %d)", errno);
    }

    pthread_mutex_init(&level_m, NULL);
    init_level_data(&level_d);

    init_billing(&billing);
    pthread_mutex_init(&billing_m, NULL);
 
    pthread_t * entrance_threads = malloc(sizeof(pthread_t) * ENTRANCES);
    pthread_t * exit_threads = malloc(sizeof(pthread_t) * EXITS);
    pthread_t * level_threads = malloc(sizeof(pthread_t) * LEVELS); 

    entrance_args_t entrance_args[5];
    level_args_t level_args[5];
    exit_args_t exit_args[5];

    // start exit threads
    for(int i = 0; i < EXITS; i++){

        exit_args[i].thread_num = i+1; 
        exit_args[i].shared_mem = shm; 
        exit_args[i].billing_m = &billing_m;
        exit_args[i].billing = &billing;
        exit_args[i].billing_fp = billing_fp;
        exit_args[i].total_bill = &total_bill;
        exit_args[i].level_m = &level_m;
        exit_args[i].level_d = &level_d;
        exit_args[i].free_spots = &free_spots;
        pthread_create(exit_threads+i, NULL, &exit_thread, &exit_args[i]);         
    }

    // start level threads
    for(int i = 0; i < LEVELS; i++){
        level_args[i].thread_num = i+1;
        level_args[i].shared_mem = shm;
        level_args[i].level_m = &level_m;
        level_args[i].level_d = &level_d;
        pthread_create(exit_threads+i, NULL, &level_thread, &level_args[i]);         
    }
 
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
