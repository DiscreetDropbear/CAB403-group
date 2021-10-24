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

// max number of entrances, exits and levels is 5, 
// always just have 5 elements in this array
// as 5 ints is small
int thread_number[5] = {1, 2, 3, 4, 5};

// no need to lock this because after the values are written into the list 
// in main they won't change and there will be only reads
Map allow_list;

// gate opening times
// only one thread will be reading and writing the
// same value so no locks are required
// i.e entrance 1 only uses entrance_gate_times[0]
//     entrance 2 only uses entrance_gate_times[1]
//     etc.
struct timespec entrance_gate_times[ENTRANCES];
struct timespec exit_gate_times[EXITS];

// level_m protexts level_d and free_spots
pthread_mutex_t level_m;
level_data_t level_d;
size_t free_spots;

pthread_mutex_t billing_m;
billing_t billing;

volatile void * shm; 

void* entrance_thr(void* arg){
    int tn = *(int*)arg;    

    // TODO: I think this is the only place that there aren't assurances on the
    // order of the threads since there is a race condition between this process
    // getting the lock and sending the signal and the simulator getting the lock
    // to wait on the signal so technicaly the signal could be missed, the simulator
    // should be started first and given sometime to setup the memory and wait on the
    // signal one second should be more than long enough but if we can we should try 
    // and make this infalible possibly using some of the padding in the shared memory to
    // signify when the simulator has the lock, this way the manager can go in a loop getting
    // the lock and checking that value untill it is set in a way in which the simulator has
    // already had the lock which means it is currently waiting on the signal
    //
    // signal the simluator thread that we are ready, then wait on the entrance lpr 
    // condition variable
    // LOCK LPR
    pthread_mutex_lock(&ENTRANCE_LPR(tn,shm)->m); 

    // signal the simluator that we are ready
    pthread_cond_signal(&ENTRANCE_LPR(tn, shm)->c);
    
    while(1){
        // UNLOCK LPR
        pthread_cond_wait(&ENTRANCE_LPR(tn, shm)->c, &ENTRANCE_LPR(tn, shm)->m);

        // LOCK LPR     
        pthread_mutex_lock(&ENTRANCE_LPR(tn,shm)->m); 

        char * rego = calloc(7, sizeof(char));
        assert(rego != NULL);
            
        // read rego 
        memcpy(rego, &ENTRANCE_LPR(tn, shm)->rego, 6);
        rego[6] = '\0';
        
        // check if the rego is on the allow list 
        if(!exists(&allow_list, rego)){
            // rego not allowed in, show 'X' on the sign
            pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm)->m);
            ENTRANCE_SIGN(tn, shm)->display = 'X';
            pthread_cond_signal(&ENTRANCE_SIGN(tn, shm)->c);
            pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm)->m);

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

            pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm)->m);
            ENTRANCE_SIGN(tn, shm)->display = 'X';
            pthread_cond_signal(&ENTRANCE_SIGN(tn, shm)->c);
            pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm)->m);

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
        pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm)->m);
        ENTRANCE_SIGN(tn, shm)->display = level_char;
        pthread_cond_signal(&ENTRANCE_SIGN(tn, shm)->c);
        pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm)->m);

        // LOCK BOOM GATE
        pthread_mutex_lock(&ENTRANCE_BOOM(tn, shm)->m);
        
        // boom gate is set to closed
        if(ENTRANCE_BOOM(tn, shm)->state == 'C'){
            // set gate to raising
            ENTRANCE_BOOM(tn, shm)->state = 'R';     
            pthread_cond_signal(&ENTRANCE_BOOM(tn, shm)->c);
            // wait for the singal that the gate is set to open
            pthread_cond_wait(&ENTRANCE_BOOM(tn, shm)->c, &ENTRANCE_BOOM(tn, shm)->m);
            
            int res = clock_gettime(CLOCK_MONOTONIC, &entrance_gate_times[tn-1]);
           
            if(res != 0){
                fprintf(stderr, "error getting the current time");
                exit(-1); 
            }
        } 
        else{ // boom gate is set to open 
            // if gate has been open for more than 20 ms
            unsigned long open_duration;
            int res = time_diff(entrance_gate_times[tn-1], &open_duration);

            if(open_duration >= 20){
                // gate has been open for too long, set to lowering
                ENTRANCE_BOOM(tn, shm)->state = 'L';     

                // send signal stating that the state is ready to be read
                pthread_cond_signal(&ENTRANCE_BOOM(tn, shm)->c);

                // wait for the signal that the gate is set to closed 
                // UNLOCKS THE BOOM GATE
                pthread_cond_wait(&ENTRANCE_BOOM(tn, shm)->c, &ENTRANCE_BOOM(tn, shm)->m);
                
                // get the lock again so we can change the state and wait on the signal
                // again
                pthread_mutex_lock(&ENTRANCE_BOOM(tn, shm)->m);
                
                // set the state to raising
                ENTRANCE_BOOM(tn, shm)->state = 'R';
                
                // wait for the signal that the gate is set to open 
                // UNLOCKS THE BOOM GATE
                pthread_cond_wait(&ENTRANCE_BOOM(tn, shm)->c, &ENTRANCE_BOOM(tn, shm)->m);

                // save the time that the gate was opened
                int res = clock_gettime(CLOCK_MONOTONIC, &entrance_gate_times[tn-1]);
               
                if(res != 0){
                    fprintf(stderr, "error getting the current time");
                    exit(-1); 
                }
            }
            else{ // leave the gate open
                 
                // send the signal so the simulator can read the boomgates
                // state
                pthread_cond_signal(&ENTRANCE_BOOM(tn, shm)->c);
                // manually release the boom gate lock
                pthread_mutex_unlock(&ENTRANCE_BOOM(tn, shm)->m);
            }
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
    pthread_mutex_init(&billing_m, NULL);

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
