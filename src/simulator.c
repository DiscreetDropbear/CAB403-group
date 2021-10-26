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
#include "libs/queue.h"
#include "libs/macros.h"

// max number of entrances, exits and levels is 5, always just have 5 elements as 5 ints is small
int thread_number[5] = {1, 2, 3, 4, 5};

shared_queue entrance_queues[ENTRANCES];
shared_queue exit_queues[EXITS];
    
pthread_mutex_t map_m;   
Map inside;
Map outside;
Map allow_list;

// sets up both the mutex's and queues for the entrances and exits
void init_shared_queues(){
    for(int i = 0; i<=ENTRANCES; i++){
        pthread_mutex_init(&entrance_queues[i].m, NULL); 
        init_queue(&entrance_queues[i].q);
    }

    for(int i = 0; i<=EXITS; i++){
        pthread_mutex_init(&exit_queues[i].m, NULL); 
        init_queue(&exit_queues[i].q);
    }
}

void init_maps(){
    pthread_mutex_init(map_m, NULL);
    init_map(inside);
    init_map(outside);

    // get allowed regos

    
    // insert them into outside map
}

int main() {

    /// setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }
    ftruncate(shm_fd, 2920);
	volatile void * shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    /// setup the mutex's and condition variables for use between processes
    init_shared_mem(shm);

    init_shared_queues();
    init_maps();

	pthread_t * entrance_threads = malloc(sizeof(pthread_t) * ENTRANCES);
    pthread_t * exit_threads = malloc(sizeof(pthread_t) * EXITS);
    pthread_t * spawner_thread = malloc(sizeof(pthread_t)); 

    /// start car entry threads (one per entry)
    for(int i = 0; i < ENTRANCES; i++){
        pthread_create(entrance_threads+i, NULL, &entrance_queue, &thread_number[i]);         
    }

    /// start car exit threads (one per exit)
    for(int i = 0; i < EXITS; i++){
        pthread_create(exit_threads+i, NULL, &exit_thr, &thread_number[i]);         
    }

    /// start car spawner thread
    pthread_create(spawner_thread, NULL, &spawner, NULL);         
    
    //
    /// wait for all threads to exit
    //
    void*retval;
    for(int i = 0; i < ENTRANCES; i++){
        pthread_join(*(entrance_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }

    pthread_join(*spawner_thread, &retval);  

    //
    /// CLEAN UP
    //

    /// cleanup the queues used won't bother locking as all other threads
    /// are now dead
    for(int i = 0; i < EXITS; i++){
        free_queue_nodes(&exit_queues[i].q);
    }

    for(int i = 0; i < ENTRANCES; i++){
        free_queue_nodes(&entrance_queues[i].q);
    }

    /// close shared memory
    shm_unlink("PARKING");

    printf("SIMULATOR FINISHED\n");
    return 0;
}
