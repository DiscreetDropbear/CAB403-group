#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
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


void wait_on_lpr(struct lpr_t* lpr, struct boom_t* boom, bool* open, struct timespec gate_close_time){
    int rt;

    do{
        if(*open){
            rt = pthread_cond_timedwait(&lpr->c, &lpr->m, &gate_close_time);
            
            // simulator cant have boom lock?
            if(rt == ETIMEDOUT){
                pthread_mutex_lock(&boom->m);
                boom->state = 'L';     
                while(boom->state != 'C'){
                    pthread_cond_broadcast(&boom->c);
                    pthread_cond_wait(&boom->c, &boom->m);
                }
                *open = false;
                pthread_mutex_unlock(&boom->m);
            }
            if(rt != 0 && rt != ETIMEDOUT){
                fprintf(stderr, "pthread_cond_timedwait failed with: %d\n", rt);
                exit(-1);
            }
        }
        else{

            // UNLOCK LPR
            pthread_cond_wait(&lpr->c, &lpr->m);
            rt = 0;
        }
    }while(rt != 0);  
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

    struct boom_t *boom = EXIT_BOOM(tn, shm);
    struct lpr_t *lpr = EXIT_LPR(tn, shm);
    struct timespec gate_close_time;

    unsigned long duration_ms; 
    char * rego = calloc(7, sizeof(char));
    rego[6] = '\0';
    assert(rego != NULL);

    struct timespec wait_time;
    bool open = false; 
    unsigned long open_duration;
    int rt = 0;

    int count = 0;

    pthread_mutex_lock(&lpr->m);

    while(1){

        memcpy(&lpr->rego, "------", 6);
        wait_on_lpr(lpr, boom, &open, gate_close_time);

        // read rego 
        memcpy(rego, &lpr->rego, 6);

        // LOCK LEVEL DATA
        pthread_mutex_lock(level_m); 
        // a car is leaving so we can:  
        // increase the free spots in the car park
        // and free up one of the spots in a level
        (*free_spots) += 1;
        remove_from_all_levels(level_d, rego);
        pthread_mutex_unlock(level_m);

        // RELEASE LEVEL DATA
 
        // GET BILLING LOCK
        pthread_mutex_lock(billing_m);
        // remove rego from billing data and calcualate bill
        int res = remove_rego(billing, rego, &duration_ms);
        assert(res == 0);

        insert(&billing->existed, rego, NULL);
        // bill is in cents
        unsigned int bill = duration_ms * 5;
        *total_bill += bill;
        
        if(save_billing(billing_fp ,rego, bill) != 0){
            fprintf(stderr, "error saving billing information to file");
            abort();
        }
        pthread_mutex_unlock(billing_m);

        // RELEASE BILLING LOCK
       
        pthread_mutex_lock(&boom->m);
        // signal simulator that removing the rego from all of the 
        // data structures is done and it can procede
        // This avoids a race condition of the manager not being able
        // to remove the regos from the billing and level data before
        // the simulator tries to send that rego into the system again
        pthread_cond_signal(&lpr->c);
        pthread_cond_wait(&lpr->c, &lpr->m);

        if(boom->state == 'O' && has_past(gate_close_time)){
            boom->state = 'L';     
            while(boom->state != 'C'){
                pthread_cond_broadcast(&boom->c);
                pthread_cond_wait(&boom->c, &boom->m);
            }
            open = false;
        }

        if(boom->state == 'C'){
            boom->state = 'R';     
            while(boom->state != 'O'){
                pthread_cond_broadcast(&boom->c);
                pthread_cond_wait(&boom->c, &boom->m);
            }
            future_time(&gate_close_time, 20);
            open = true;   
        }
        pthread_mutex_unlock(&boom->m);
    }
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

    bool open = false; 
    unsigned long open_duration;
    struct timespec gate_close_time;

    struct boom_t* boom = ENTRANCE_BOOM(tn, shm);
    struct lpr_t* lpr = ENTRANCE_LPR(tn, shm);
    struct sign_t* sign = ENTRANCE_SIGN(tn, shm);

    char * rego = calloc(7, sizeof(char));
        
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
    pthread_mutex_lock(&lpr->m); 

    // signal the simluator that we are ready
    pthread_cond_signal(&lpr->c);
    int rt;

    int count = 0;

    while(1){
        // set lpr and sign
        memcpy(&lpr->rego, "------", 6);

        pthread_mutex_lock(&sign->m);
        if(sign->display != 'X'){
            sign->display = ' ';
        }
        pthread_mutex_unlock(&sign->m);

        // 
        wait_on_lpr(lpr, boom, &open, gate_close_time);


        // read rego 
        memcpy(rego, &lpr->rego, 6);

        // check if the rego is on the allow list 
        if(!exists(allow_list, rego)){
            // rego not allowed in, show 'X' on the sign
            pthread_mutex_lock(&sign->m);
            sign->display = 'X';
            pthread_cond_signal(&sign->c);
            pthread_cond_wait(&sign->c, &sign->m);
            pthread_mutex_unlock(&sign->m);
            pthread_mutex_unlock(&boom->m);
            continue;
        }
       
        pthread_mutex_lock(level_m);// LOCK LEVEL DATA     
        assert(*free_spots >= 0);
        pthread_mutex_unlock(level_m);// UNLOCK LEVEL DATA
        // no spaces in the car park show 'F' on the sign
        if(*free_spots == 0){
            // UNLOCK LEVEL DATA
            pthread_mutex_unlock(level_m);

            pthread_mutex_lock(&sign->m);
            sign->display = 'F';
            pthread_cond_signal(&sign->c);
            pthread_cond_wait(&sign->c, &sign->m);
            pthread_mutex_unlock(&sign->m);
            pthread_mutex_unlock(&boom->m);
            continue;
        }

        pthread_mutex_lock(level_m);// LOCK LEVEL DATA     
        (*free_spots) -= 1;;
        uint8_t level = get_available_level(level_d); 

        assert(level != 0);
        insert_in_level(level_d, level, rego);
        pthread_mutex_unlock(level_m);// UNLOCK LEVEL DATA
       

        pthread_mutex_lock(billing_m);// LOCK BILLING DATA        
        insert_rego(billing, rego);  
        pthread_mutex_unlock(billing_m);// UNLOCK BILLING DATA

        // The car has been assigned a level now show that 
        // level on the sign and signal the car to read the 
        // sign
        
        pthread_mutex_lock(&boom->m);
        // ascii encode the level number
        pthread_mutex_lock(&sign->m);
        sign->display = 48 + level;
        pthread_cond_signal(&sign->c);
        pthread_cond_wait(&sign->c, &sign->m);
        pthread_mutex_unlock(&sign->m);

        
        if(boom->state == 'O' && has_past(gate_close_time)){
            boom->state = 'L';     
            while(boom->state != 'C'){
                pthread_cond_broadcast(&boom->c);
                pthread_cond_wait(&boom->c, &boom->m);
            }
            open = false;
        }

        if(boom->state == 'C'){
            boom->state = 'R';     
            while(boom->state != 'O'){
                pthread_cond_broadcast(&boom->c);
                pthread_cond_wait(&boom->c, &boom->m);
            }
            future_time(&gate_close_time, 20);
            open = true;   
        }

        // manually release the boom gate lock
        pthread_mutex_unlock(&boom->m);
    }
}



void* level_thread(void* _args){
    level_args_t* args = _args; 

    int tn = args->thread_num;
    volatile void* shm = args->shm;
    pthread_mutex_t* level_m = args->level_m;
    level_data_t* level_d = args->level_d;

    struct lpr_t* lpr = LEVEL_LPR(tn, shm);

    // when a car enters the level that isn't assigned to this level
    // and there is no room to assign them they are inserted into this 
    // map so we can tell when they are exiting
    Map entered;
    init_map(&entered, 0);
    int num = 0;

    char * rego = calloc(7, sizeof(char));
    assert(rego != NULL);
    int count = 0;
     
    // get lock for level lpr
    pthread_mutex_lock(&lpr->m);  
    
    while(1){
        //memcpy(&lpr->rego, "------", 6);
        // wait on the lpr signal
        // RELEASES LPR LOCK
        pthread_cond_wait(&lpr->c, &lpr->m);
        // LPR IS LOCKED after waking up from cond wait 

        // read rego 
        memcpy(rego, &lpr->rego, 6);
        rego[6] = '\0';
        
        // LOCK LEVEL DATA 
        pthread_mutex_lock(level_m);

        // check if rego is not assigned to this level
        if(!exists_in_level(level_d, tn, rego)){
            // check if we have space in this level 
            // even if the car is exiting the level and there wasn't space before
            // so it wasn't added to this level but now there is space so it's being
            // added now it won't screw anything up and it will also update the 
            // other level that it actually has space faster than when the car exits 
            // the whole car park 
            if(cars_in_level(level_d, tn) < LEVEL_CAPACITY){
                // remove from other level
                remove_from_all_levels(level_d, rego);
                // assign to this level
                insert_in_level(level_d, tn, rego);
            }
        }

        // UNLOCK LEVEL DATA
        pthread_mutex_unlock(level_m);
        pthread_cond_signal(&lpr->c);
    }
}

void* display_thread(void* _args){
    if(DISPLAY_THREAD == 0){
        return NULL;
    }
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
    int level_alarms[LEVELS];
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

            level_alarms[i] = *LEVEL_ALARM(i+1, shm);

            pthread_mutex_lock(level_m);
            level_caps[i] = cars_in_level(level_d, i+1);
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
        printf("level number\tspots\tlpr\ttemp\talarm\n");
        for(int i = 0; i<LEVELS; i++){
            printf("%d           \t%d/%d\t%s\t%d\t%s\n", i+1,level_caps[i], LEVEL_CAPACITY, l_lpr[i], level_temps[i], level_alarms[i] == 0 ? "Off":"On"); 

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

        usleep(10*1000); // sleep for 50 ms
    }
}
