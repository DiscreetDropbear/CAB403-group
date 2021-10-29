#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "include/types.h"
#include "include/queue.h"
#include "include/macros.h"
#include "include/map.h"
#include "include/simulator_funcs.h"
#include "include/utils.h"

pthread_mutex_t rand_m;  
pthread_mutex_t outer_level_m[LEVELS];  
int thread_number[5] = {1, 2, 3, 4, 5};

shared_queue_t entrance_queues[ENTRANCES];
shared_queue_t exit_queues[EXITS];

maps_t maps;
Map allow_list;

// sets up both the mutex's and queues for the entrances and exits
void init_shared_queues(){
    
    pthread_condattr_t pcond_attr;
    (void) pthread_condattr_init(&pcond_attr);
    (void) pthread_condattr_setclock(&pcond_attr, CLOCK_MONOTONIC);

    for(int i = 0; i<ENTRANCES; i++){
        pthread_mutex_init(&entrance_queues[i].m, NULL); 
        pthread_cond_init(&entrance_queues[i].c, &pcond_attr);
        init_queue(&entrance_queues[i].q);
    }

    for(int i = 0; i<EXITS; i++){
        pthread_mutex_init(&exit_queues[i].m, NULL);
        pthread_cond_init(&exit_queues[i].c, &pcond_attr);
        init_queue(&exit_queues[i].q);
    }
}

void init_maps(){
    pthread_mutex_init(&maps.m, NULL);
    
    char ** regos;
    int num_regos;

    if(load_regos(&regos, &num_regos) != 0){
        fprintf(stderr, "error retreiving regos from plates.txt"); 
        exit(-1);
    }

    init_map(&maps.inside, num_regos);
    init_map(&maps.outside, num_regos);
    init_map(&allow_list, num_regos);

    // insert all of the allowed regos into the outside map
    // and the allow list
    for(int i = 0; i < num_regos; i++){
        // this copies the regos so we still have to free our copy
        insert(&maps.outside, regos[i], NULL);
        insert(&allow_list, regos[i], NULL);
    }

    // free the memory for regos
    for(int i = 0; i< num_regos; i++){
        free(regos[i]);
    }
    free(regos);
}

int main() {
    pthread_t boom_threads[ENTRANCES+EXITS];
    pthread_t entrance_threads[ENTRANCES];
    pthread_t exit_threads[EXITS];
    pthread_t generator_thread; 
    pthread_t temp_thread;

    temp_args_t temp_args;
    generator_args_t gen_args;
    entr_args_t entr_args[ENTRANCES];
    exit_args_t exit_args[EXITS];

    // set the seed for rand 
    srand(time(0));

    //
    /// setup shared memory
    //
	int shm_fd = shm_open("PARKING", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }
    // resize shared memory
    ftruncate(shm_fd, 2920);
	volatile void * shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    // setup the mutex's and condition variables for use between processes
    init_shared_mem(shm);

    // setup the mutex to protect rand accesses
    pthread_mutex_init(&rand_m, NULL);
    for(int i=0; i< LEVELS; i++){
        pthread_mutex_init(&outer_level_m[i], NULL);
    }
    // setup the queues
    init_shared_queues();
    // setup the maps and the maps mutex
    init_maps();


	/// start car entry threads (one per entry)
    for(int i = 0; i < ENTRANCES; i++){
        entr_args[i].shm = shm;
        entr_args[i].entrance = i+1;
        entr_args[i].entrance_queue = &entrance_queues[i]; 
        entr_args[i].exit_queues = &exit_queues[0];
        entr_args[i].maps = &maps;
        entr_args[i].allow_list = &allow_list;
        entr_args[i].rand_m = &rand_m;
        entr_args[i].outer_level_m = &outer_level_m[0];

        pthread_create(&entrance_threads[i], NULL, &entrance_queue, &entr_args[i]);         
        pthread_create(&boom_threads[i], NULL, &boom_thread, ENTRANCE_BOOM(i+1, shm));
    }

    /// start car exit threads (one per exit)
    for(int i = 0; i < EXITS; i++){
        exit_args[i].exit = i+1;
        exit_args[i].shm = shm;
        exit_args[i].allow_list = &allow_list;
        exit_args[i].exit_queue = &exit_queues[i];
        exit_args[i].maps = &maps;
        pthread_create(&exit_threads[i], NULL, &exit_thr, &exit_args[i]);         
        pthread_create(&boom_threads[ENTRANCES-1+i], NULL, &boom_thread, EXIT_BOOM(i+1, shm));
    }

    /// start car generator thread
    gen_args.entrance_queues = &entrance_queues[0];
    gen_args.exit_queues = &exit_queues[0];
    gen_args.maps = &maps;
    gen_args.rand_m = &rand_m;
    pthread_create(&generator_thread, NULL, &generator, &gen_args);         

    // start temperature changing thread
    temp_args.shm = shm;
    temp_args.rand_m = &rand_m;
    pthread_create(&temp_thread, NULL, &temp_setter, &temp_args);         
    //
    /// wait for all threads to exit
    //
    void*retval;
    for(int i = 0; i < ENTRANCES; i++){
        pthread_join(entrance_threads[i], &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(exit_threads[i], &retval);  
    }

    pthread_join(generator_thread, &retval);  

    //
    /// CLEAN UP
    //

    /// cleanup the queues used won't bother locking as all other threads
    /// are now dead
    for(int i = 0; i < EXITS; i++){
//        free_queue_nodes(&exit_queues[i].q);
    }

    for(int i = 0; i < ENTRANCES; i++){
 //       free_queue_nodes(&entrance_queues[i].q);
    }

    /// close shared memory
    shm_unlink("PARKING");
    return 0;
}
