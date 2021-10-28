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

// TODO: test this function
int save_billing(FILE* fp ,char* rego, unsigned int bill){
    assert(fp != NULL); 
    unsigned int dollars = bill/100;
    unsigned int cents = bill-(dollars*100);

    int res = fprintf(fp, "%s $%u.%u\n", rego, bill/100, bill-(bill/100)); 
    fflush(fp);

    if(res < 0){
        return -1; 
    }

    return 0;
}

void* entrance_thread(void* _args){
    struct entrance_args* args = _args;
    int tn = args->thread_num;
    volatile void* shm = args->shm;
    Map* allow_list = args->allow_list;
    pthread_mutex_t* billing_m = args->billing_m;
    billing_t* billing = args->billing;
    pthread_mutex_t* level_m = args->level_m; 
    level_data_t* level_d = args->level_d;
    size_t* free_spots = args->free_spots;
    struct timespec gate_time;

    char * rego = calloc(7, sizeof(char));
    assert(rego != NULL);
        
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
        // set lpr and sign
        memcpy(&ENTRANCE_LPR(tn, shm)->rego, "------", 6);
        if(ENTRANCE_SIGN(tn, shm)->display != 'X'){
            ENTRANCE_SIGN(tn, shm)->display = ' ';
        }

        // UNLOCK LPR
        pthread_cond_wait(&ENTRANCE_LPR(tn, shm)->c, &ENTRANCE_LPR(tn, shm)->m);

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
        assert(*free_spots >= 0);
        // no spaces in the car park show 'X' on the sign
        if(*free_spots == 0){
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

        (*free_spots) -= 1;;
        size_t level = get_available_level(level_d); 

        assert(level != 0);
        insert_in_level(level_d, level, rego);
        pthread_mutex_unlock(level_m);// UNLOCK LEVEL DATA
        
        pthread_mutex_lock(billing_m);// LOCK BILLING DATA        
        insert_rego(billing, rego);  
        pthread_mutex_unlock(billing_m);// UNLOCK BILLING DATA

        // The car has been assigned a level now show that 
        // level on the sign and signal the car to read the 
        // sign
        
        // ascii encode the level number
        char level_char = '0' + level; 
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
            // wait for the signal that the gate is set to open
            while(ENTRANCE_BOOM(tn, shm)->state != 'O'){
                pthread_cond_wait(&ENTRANCE_BOOM(tn, shm)->c, &ENTRANCE_BOOM(tn, shm)->m);
            }
            pthread_mutex_unlock(&ENTRANCE_BOOM(tn, shm)->m);
            
            int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
           
            if(res != 0){
                fprintf(stderr, "error getting the current time");
                abort(); 
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
                
                // set the state to raising
                ENTRANCE_BOOM(tn, shm)->state = 'R';
                
                pthread_cond_signal(&ENTRANCE_BOOM(tn, shm)->c);
                // wait for the signal that the gate is set to open 
                // UNLOCKS THE BOOM GATE
                pthread_cond_wait(&ENTRANCE_BOOM(tn, shm)->c, &ENTRANCE_BOOM(tn, shm)->m);
                pthread_mutex_unlock(&ENTRANCE_BOOM(tn, shm)->m);
                // save the time that the gate was opened
                int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
               
                if(res != 0){
                    fprintf(stderr, "error getting the current time");
                    abort(); 
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

void* exit_thread(void* _args){
    struct exit_args* args = _args;
    int tn = args->thread_num;

    volatile void* shm = args->shm;

    pthread_mutex_t* billing_m = args->billing_m;
    billing_t* billing = args->billing;
    FILE* billing_fp = args->billing_fp;
    unsigned int* total_bill = args->total_bill;

    pthread_mutex_t* level_m = args->level_m; 
    level_data_t* level_d = args->level_d;
    size_t* free_spots = args->free_spots;

    struct timespec gate_time;

    unsigned long duration_ms; 
    char * rego = calloc(7, sizeof(char));
    rego[6] = '\0';
    assert(rego != NULL);

    pthread_mutex_lock(&EXIT_LPR(tn,shm)->m); 

    while(1){

        memcpy(&EXIT_LPR(tn, shm)->rego, "------", 6);
        // UNLOCK LPR
        pthread_cond_wait(&EXIT_LPR(tn, shm)->c, &EXIT_LPR(tn, shm)->m);
            
        // read rego 
        memcpy(rego, &EXIT_LPR(tn, shm)->rego, 6);

        // GET BILLING LOCK
        pthread_mutex_lock(billing_m);
        // remove rego from billing data and calcualate bill
        int res = remove_rego(billing, rego, &duration_ms);
        if(res != 0){
            fprintf(stderr, "existed before is %s\n", exists(&billing->existed, rego)? "True":"False"); 
            fprintf(stderr, "%s\n", exists(&billing->map, rego)? "True":"False"); 
        }
        assert(res == 0);

        insert(&billing->existed, rego, NULL);
        // bill is in cents
        unsigned int bill = duration_ms * 5;
        *total_bill += bill;
        
        if(save_billing(billing_fp ,rego, bill) != 0){
            fprintf(stderr, "error saving billing information to file");
            abort();
        }
        
        // RELEASE BILLING LOCK
        pthread_mutex_unlock(billing_m);

        // LOCK LEVEL DATA
        pthread_mutex_lock(level_m); 
        // a car is leaving so we can:  
        // increase the free spots in the car park
        // and free up one of the spots in a level
        (*free_spots) += 1;
        remove_from_all_levels(level_d, rego);
        
        // RELEASE LEVEL DATA LOCK
        pthread_mutex_unlock(level_m);

        // LOCK BOOM GATE
        pthread_mutex_lock(&EXIT_BOOM(tn, shm)->m);
        
        // boom gate is set to closed
        if(EXIT_BOOM(tn, shm)->state == 'C'){
            // set gate to raising
            EXIT_BOOM(tn, shm)->state = 'R';     
            pthread_cond_signal(&EXIT_BOOM(tn, shm)->c);
            // wait for the singal that the gate is set to open
            pthread_cond_wait(&EXIT_BOOM(tn, shm)->c, &EXIT_BOOM(tn, shm)->m);
            pthread_mutex_unlock(&EXIT_BOOM(tn, shm)->m);
            
            int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
           
            if(res != 0){
                fprintf(stderr, "error getting the current time");
                abort();
            }
        } 
        else{ // boom gate is set to open 
            // if gate has been open for more than 20 ms
            unsigned long open_duration;
            int res = time_diff(gate_time, &open_duration);

            if(open_duration >= 20){
                // gate has been open for too long, set to lowering
                EXIT_BOOM(tn, shm)->state = 'L';     

                // send signal stating that the state is ready to be read
                pthread_cond_signal(&EXIT_BOOM(tn, shm)->c);

                // wait for the signal that the gate is set to closed 
                // UNLOCKS THE BOOM GATE
                pthread_cond_wait(&EXIT_BOOM(tn, shm)->c, &EXIT_BOOM(tn, shm)->m);
                
                // set the state to raising
                EXIT_BOOM(tn, shm)->state = 'R';
                
                pthread_cond_signal(&EXIT_BOOM(tn, shm)->c);
                // wait for the signal that the gate is set to open 
                // UNLOCKS THE BOOM GATE
                pthread_cond_wait(&EXIT_BOOM(tn, shm)->c, &EXIT_BOOM(tn, shm)->m);
                pthread_mutex_unlock(&EXIT_BOOM(tn, shm)->m);

                // save the time that the gate was opened
                int res = clock_gettime(CLOCK_MONOTONIC, &gate_time);
               
                if(res != 0){
                    fprintf(stderr, "error getting the current time");
                    abort();
                }
            }
            else{ // leave the gate open
                 
                // send the signal so the simulator can read the boomgates
                // state
                pthread_cond_signal(&EXIT_BOOM(tn, shm)->c);
                // manually release the boom gate lock
                pthread_mutex_unlock(&EXIT_BOOM(tn, shm)->m);
            }
        }
    }
}

void* level_thread(void* _args){
    level_args_t* args = _args; 

    int tn = args->thread_num;
    volatile void* shm = args->shm;
    pthread_mutex_t* level_m = args->level_m;
    level_data_t* level_d = args->level_d;

    // when a car enters the level that isn't assigned to this level
    // and there is no room to assign them they are inserted into this 
    // map so we can tell when they are exiting
    Map entered;
    init_map(&entered, 0);

    char * rego = calloc(7, sizeof(char));
    assert(rego != NULL);
     
    // get lock for level lpr
    pthread_mutex_lock(&LEVEL_LPR(tn, shm)->m);  
    
    while(1){

        usleep(10*1000);
        memcpy(&LEVEL_LPR(tn, shm)->rego, "------", 6);
        // wait on the lpr signal
        // RELEASES LPR LOCK
        pthread_cond_wait(&LEVEL_LPR(tn, shm)->c, &LEVEL_LPR(tn, shm)->m);
        // LPR IS LOCKED after waking up from cond wait 

        // read rego 
        memcpy(rego, &LEVEL_LPR(tn, shm)->rego, 6);
        rego[6] = '\0';
        
        // LOCK LEVEL DATA 
        pthread_mutex_lock(level_m);
            
        // check if rego is not assigned to this level
        if(!exists_in_level(level_d, tn, rego) && !exists(&entered, rego)){
            // check if we have space in this level 
            if(levels_free_parks(level_d, tn) > 0){
                // remove from other level
                remove_from_all_levels(level_d, rego);
                // assign to this level
                insert_in_level(level_d, tn, rego);
            }
            else{
                // there is no space on this level, record the fact that this car is entering
                insert(&entered, rego, NULL);
            }
        }
        else if(exists(&entered, rego)){
            // this car has already entered the level but they werene't assigned to this level
            // because there was no space, they are leaving now so all we need to do
            // is remove them from the entered map
            (void)remove_key(&entered, rego);
        }

        // UNLOCK LEVEL DATA
        pthread_mutex_unlock(level_m);

        pthread_cond_signal(&LEVEL_LPR(tn, shm)->c);
    }
}

void* display_thread(void* _args){
//    return NULL;
    display_args_t * args = _args;        
    volatile void* shm = args->shm;

    pthread_mutex_t* billing_m = args->billing_m;
    unsigned int* total_bill = args->bill;
    unsigned int saved_bill;
    unsigned int dollars;
    unsigned int cents;

    pthread_mutex_t* level_m = args->level_m; 
    level_data_t* level_d = args->level_d;
    size_t* free_spots = args->free_spots;

    // set up entrance variables
    char ** entr_lpr = calloc(ENTRANCES, sizeof(char*));    
    char entr_boom[ENTRANCES];
    char sign[ENTRANCES];
    for(int i = 0; i<ENTRANCES; i++){
        entr_lpr[i] = calloc(7, sizeof(char)); 
    }

    // set up level variables
    char ** l_lpr = calloc(LEVELS, sizeof(char*));    
    short level_temps[LEVELS];
    for(int i = 0; i<LEVELS; i++){
        l_lpr[i] = calloc(7, sizeof(char)); 
    }
    
    // set up exit variables 
    char ** exit_lpr = calloc(EXITS, sizeof(char*));    
    for(int i = 0; i<EXITS; i++){
        exit_lpr[i] = calloc(7, sizeof(char)); 
    }
    char exit_boom[EXITS];

    int level_caps[LEVELS];

    while(1){
        // get all data for entrances 
        for(int i = 0; i<ENTRANCES; i++){
            //pthread_mutex_lock(&ENTRANCE_LPR(i+1, shm)->m);
            memcpy(entr_lpr[i], &ENTRANCE_LPR(i+1, shm)->rego, 6); 
            //pthread_mutex_unlock(&ENTRANCE_LPR(i+1,shm)->m);

            // boomgate's
            //pthread_mutex_lock(&ENTRANCE_BOOM(i+1, shm)->m);
            entr_boom[i] = ENTRANCE_BOOM(i+1, shm)->state;
            //pthread_mutex_unlock(&ENTRANCE_BOOM(i+1, shm)->m);

            // sign
            //pthread_mutex_lock(&ENTRANCE_SIGN(i+1, shm)->m);
            sign[i] = ENTRANCE_SIGN(i+1, shm)->display;            
            //pthread_mutex_unlock(&ENTRANCE_SIGN(i+1, shm)->m);
        }

        // get all data for levels 
        for(int i = 0; i<LEVELS; i++){
            // lpr's
            //pthread_mutex_lock(&LEVEL_LPR(i+1, shm)->m);
            memcpy(l_lpr[i], &LEVEL_LPR(i+1, shm)->rego, 6);   
            //pthread_mutex_unlock(&LEVEL_LPR(i+1, shm)->m);
            
            // temperatures
            level_temps[i] = *LEVEL_TEMP(i+1, shm);

            pthread_mutex_lock(level_m);
            level_caps[i] = LEVEL_CAPACITY - levels_free_parks(level_d, i+1);
            pthread_mutex_unlock(level_m);
        }

        // get all data for exits
        for(int i = 0; i<EXITS; i++){
            //pthread_mutex_lock(&EXIT_LPR(i+1, shm)->m); 
            memcpy(exit_lpr[i], &EXIT_LPR(i+1, shm)->rego, 6);
            //pthread_mutex_unlock(&EXIT_LPR(i+1, shm)->m);

            // boomgate's
            //pthread_mutex_lock(&EXIT_BOOM(i+1, shm)->m);
            exit_boom[i] = EXIT_BOOM(i+1, shm)->state;
            //pthread_mutex_lock(&EXIT_BOOM(i+1, shm)->m);
        }

        // get billing data
        pthread_mutex_lock(billing_m);   
        saved_bill = *total_bill;
        pthread_mutex_unlock(billing_m);

        dollars = saved_bill/100;
        cents = saved_bill-(dollars*100);

        // show the data
        system("clear");
        
        // print entrance data
        printf("entrance\tlpr\tboomgate\tsign\n"); 
        for(int i = 0; i<ENTRANCES; i++){
            printf("%d       \t%s\t%c\t%c\n", i+1, entr_lpr[i], entr_boom[i], sign[i]);
        }
        printf("\n");
        
        // print level data
        printf("level number\tlpr\ttemp\tcars inside\n");
        for(int i = 0; i<LEVELS; i++){
            printf("%d           \t%s\t%d\t%d/%d\n", i+1, l_lpr[i], level_temps[i], level_caps[i], LEVEL_CAPACITY); 
        }
        printf("\n");

        // print exit data
        printf("exit\tlpr\tBoomgate\n"); 
        for(int i = 0; i<EXITS; i++){
            printf("%d   \t%s\t%c\t%c\n", i+1, exit_lpr[i], exit_boom[i]);
        }
        // print billing data
        printf("current total earnings: $%u.%u", dollars, cents);
        printf("\n");

        usleep(50000); // sleep for 50 ms
    }
}
