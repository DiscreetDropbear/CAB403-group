#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#include "../include/macros.h"
#include "../include/types.h"
#include "../include/billing.h"
#include "../include/map.h"
#include "../include/leveldata.h"
#include "../include/utils.h"
#include "../include/manager_funcs.h"

void* entrance_thread(void* _args){
    struct entrance_args* args = _args;
    int tn = args->thread_num;
    volatile void* shm = args->shared_mem;
    Map* allow_list = args->allow_list;
    pthread_mutex_t* billing_m = args->billing_m;
    billing_t* billing = args->billing;
    pthread_mutex_t* level_m = args->level_m; 
    level_data_t* level_d = args->level_d;
    size_t* free_spots = args->free_spots;
    struct timespec gate_time;

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

        char * rego = calloc(7, sizeof(char));
        assert(rego != NULL);
            
        // read rego 
        memcpy(rego, &ENTRANCE_LPR(tn, shm)->rego, 6);
        rego[6] = '\0';
        
        // check if the rego is on the allow list 
        if(!exists(allow_list, rego)){
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

        
        pthread_mutex_lock(level_m);// LOCK LEVEL DATA     
        assert(free_spots >= 0);
        // no spaces in the car park show 'X' on the sign
        if(free_spots == 0){
            // UNLOCK LEVEL DATA
            pthread_mutex_unlock(level_m);

            pthread_mutex_lock(&ENTRANCE_SIGN(tn, shm)->m);
            ENTRANCE_SIGN(tn, shm)->display = 'X';
            pthread_cond_signal(&ENTRANCE_SIGN(tn, shm)->c);
            pthread_mutex_unlock(&ENTRANCE_SIGN(tn, shm)->m);

            // everything but LPR is unlocked and LPR is locked
            // ready to wait on the signal in the next iteration
            // of the loop
            continue;
        }
        size_t level = get_available_level(level_d); 
        assert(level != 0);
        insert_in_level(level_d, level, rego, false);
        pthread_mutex_unlock(level_m);// UNLOCK LEVEL DATA
        
        pthread_mutex_lock(billing_m);// LOCK BILLING DATA        
        insert_rego(billing, rego);  
        pthread_mutex_unlock(billing_m);// UNLOCK BILLING DATA

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
            
            int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
           
            if(res != 0){
                fprintf(stderr, "error getting the current time");
                exit(-1); 
            }
        } 
        else{ // boom gate is set to open 
            // if gate has been open for more than 20 ms
            unsigned long open_duration;
            int res = time_diff(gate_time, &open_duration);

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
                int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
               
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

}

void* level_thr(void* arg){

}
