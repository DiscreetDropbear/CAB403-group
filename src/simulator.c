#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "libs/types.h"
#include "libs/macros.h"

// max number of entrances, exits and levels is 5, always just have 5 elements as 5 ints is small
int thread_number[5] = {1, 2, 3, 4, 5};

shared_queue exit_queues[EXITS];
shared_queue entrance_queues[ENTRIES];

// I think only the simulator will need this so I will leave it here for now
struct shared_queue{
    pthread_mutex_t m;
    Queue q;
};

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
        pthread_mutex_init(ENTRANCE_LPR(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(ENTRANCE_LPR(i, shm).c, &pcond_attr);
        
        /// BOOM
        // setup mutex
        pthread_mutex_init(ENTRANCE_BOOM(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(ENTRANCE_BOOM(i, shm).c, &pcond_attr);
        
        /// SIGN 
        // setup mutex
        pthread_mutex_init(ENTRANCE_SIGN(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(ENTRANCE_SIGN(i, shm).c, &pcond_attr);
    }

    // EXITS
    // initates mutex's and condition variables for the exits 
    for(int i = 1; i <= EXITS; i++){
        /// LPR
        // setup mutex
        pthread_mutex_init(EXIT_LPR(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(EXIT_LPR(i, shm).c, &pcond_attr);
        
        /// BOOM
        // setup mutex
        pthread_mutex_init(ENTRANCE_BOOM(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(ENTRANCE_BOOM(i, shm).c, &pcond_attr);
    }

    // LEVELS
    // initates mutex's and condition variables for the levels
    for(int i = 1; i <= EXITS; i++){
        /// LPR
        // setup mutex
        pthread_mutex_init(EXIT_LPR(i, shm).m, &pmut_attr);
        // setup condition var
        pthread_mutex_init(EXIT_LPR(i, shm).c, &pcond_attr);
    }
}

// sets up both the mutex's and queues for the entrances and exits
void init_shared_queues(){
    for(int i = 0; i<=4; i++){
        pthread_mutex_init(&entrance_queue[i].m, NULL); 
        pthread_mutex_init(&exit_queue[i].m, NULL); 
        queue_init(&entrance_queue[i].q);
        queue_init(&exit_queue[i].q);
    }
}

void * spawner_thread(void * arg){
    
}

void * entrance_queue_thread(void * arg){
    
}

void * car_thread(void * arg){
    
}

void * exit_thread(void * arg){
    
}

int main() {
    // setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR | O_CREAT | O_TRUNC);
    // resize shared memory
    ftruncate(shm_fd, 2920);
    // memory map the shared memory
	volatile void * shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // setup the mutex's and condition variables for use between processes
    init_shared_mem(shm);
    init_shared_queues();
    
	pthread_t * entrance_threads = malloc(sizeof(pthread_t) * ENTRANCES);
    pthread_t * exit_threads = malloc(sizeof(pthread_t) * EXITS);
    pthread_t * spawner_thread = malloc(sizeof(pthread_t)); 
    // start car entry threads (one per entry)
    for(int i = 1; i <= ENTRANCES; i++){
        create_thread(&entrance_thread[i-1], NULL, &entrance_thread, &thread_number[i-1]);         
    }

    // start car exit threads (one per exit)
    for(int i = 1; i <= ENTRANCES; i++){
        create_thread(&exit_thread[i-1], NULL, &exit_thread, &thread_number[i-1]);         
    }

    // start car spawner thread
    create_thread(&spawner_thread[i-1], NULL, &spawner_thread, NULL);         
    
    // close shared memory
    return 0;
}
