#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "libs/types.h"

// max number of entrances, exits and levels is 5, 
// always just have 5 elements in this array
// as 5 ints is small
int thread_number[5] = {1, 2, 3, 4, 5};



void* entrance_thr(void* arg){
    int tn = *(int*)arg;    
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
