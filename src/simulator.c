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

// I think only the simulator will need this so I will leave it here for now
typedef struct SharedQueue{
    pthread_mutex_t m;
    Queue q;
} SharedQueue;

struct SharedQueue exit_queues[EXITS];
struct SharedQueue entrance_queues[ENTRANCES];

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

// sets up both the mutex's and queues for the entrances and exits
void init_shared_queues(){
    for(int i = 0; i<=4; i++){
        pthread_mutex_init(&entrance_queues[i].m, NULL); 
        pthread_mutex_init(&exit_queues[i].m, NULL); 
        init_queue(&entrance_queues[i].q);
        init_queue(&exit_queues[i].q);
    }
}

generate_rego()

char * get_next_rego(map * inside, map * outside){
    char * rego;

    int generate = (rand() % 2) != 0; //will provide 1 or 0

    if(generate = 1){
        while(1){
            rego = generate_rego(); 

            //will need to check whether the rego is inside or outside & that it isn't already a valid rego
        }
    }else{
        rego = select_valid_rego(outside);
    }
}

void * spawner(void * arg){
    
}

void * entrance_queue(void * arg){
    // get the thread number (1-5)
    int tn = *(int*)arg;
}


void * exit_thr(void * arg){
    // get the thread number (1-5)
    int tn = *(int*)arg;
}

void * car(void * arg){
    char * rego = (char*)arg;
}

int main() {

    /// setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }

    /// resize shared memory
    ftruncate(shm_fd, 2920);

    /// memory map the shared memory
	volatile void * shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    /// setup the mutex's and condition variables for use between processes
    init_shared_mem(shm);
    init_shared_queues();

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
    
    /// wait for all threads to exit
    void*retval;
    for(int i = 0; i < ENTRANCES; i++){
        pthread_join(*(entrance_threads+i), &retval);  
    }
    for(int i = 0; i < EXITS; i++){
        pthread_join(*(exit_threads+i), &retval);  
    }

    pthread_join(*spawner_thread, &retval);  

    /// cleanup the queues used won't bother locking as all other threads
    /// are now dead
    for(int i = 0; i < EXITS; i++){
        free_queue(&exit_queue[i].q);
    }

    for(int i = 0; i < ENTRANCES; i++){
        free_queue(&entrance_queue[i].q);
    }

    /// close shared memory
    shm_unlink("PARKING");

    printf("SIMULATOR FINISHED\n");
    return 0;
}