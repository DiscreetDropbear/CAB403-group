#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "../include/simulator_funcs.h"
#include "../include/queue.h"
#include "../include/types.h"
#include "../include/macros.h"
#include "../include/map.h"

int generate_rego(char * rego, pthread_mutex_t* rand_m){ 
    assert(rego != NULL);

    pthread_mutex_lock(rand_m);
	for (int i = 0; i < 3; i++) {
        rego[i] = rand() %(57 - 48 + 1) + 48; //char range for numbers
    }
    // Generate three random numbers
 	for (int i = 3; i < 6; i++) {
        rego[i] = rand() %(90 - 65 + 1) + 65; //char range for letter 
    }
    pthread_mutex_unlock(rand_m);

    rego[6] = '\0';
    
    return 0;
}

int select_valid_rego(Map * outside, char** rego_o, pthread_mutex_t* rand_m){

    pthread_mutex_lock(rand_m);
    int num = rand() %(100000 + 1);
    pthread_mutex_unlock(rand_m);
    pair_t pair;
    char * rego;

    pair = get_nth_item(outside, num);
    
    // failure
    if(pair.key == NULL){
        return -1; 
    }
    
    *rego_o = pair.key;
    // no need to free pair.value as it will be NULL

    // success
    return 0;
}

// returns 0 if the rego is from the allow list or 1 if it was generated
int get_next_rego(Map * inside, Map * outside, pthread_mutex_t* rand_m, char ** rego, int* valid_rego){
    char * tmp;

    pthread_mutex_lock(rand_m);
    int generate = (rand() % 10); //will provide 1 or 0
    pthread_mutex_unlock(rand_m);

    if(generate > 2){
        // a valid rego was successfully received
        if(select_valid_rego(outside, &tmp, rand_m) == 0){
            *valid_rego +=1;
            fprintf(stderr, "got %dth valid rego", *valid_rego);
            // insert the rego into the inside map
            insert(inside, tmp, NULL);
            *rego = tmp;
            return 0;
        }
    }
        
    // either generate is 1 or there are no more valid regos left
    // so we will generate a random rego

    // we need to setup the rego's memory
    tmp = calloc(7, sizeof(char));
    while(1){

        if(generate_rego(tmp, rand_m) == -1){
            fprintf(stderr, "there was an error generating a random rego\n"); 
            abort();
        }

        // make sure the rego doesn't already exist in the outside map(the allowed cars)
        // or the inside map(the cars that are already inside the carpark)
        if(!exists(inside, tmp) && !exists(outside, tmp)){
            insert(inside, tmp, NULL);
            *rego = tmp;
            return 1;
        }
    }
}

void * generator(void* _args){
    generator_args_t* args = _args;
    shared_queue_t* entr_q = args->entrance_queues; 
    shared_queue_t* exit_q = args->exit_queues;
    maps_t* maps = args->maps;
    pthread_mutex_t* rand_m = args->rand_m;
    char *rego;
    int valid_rego = 0;

    while(1){
        pthread_mutex_lock(rand_m);
        //use rand to generate number between 1 and 100 inclusive to determine spawn rate
        int st = rand() % 100 + 1;         
        int entrance = rand() % ENTRANCES;
        pthread_mutex_unlock(rand_m);

        //SLEEP(1);
        usleep(st*1000);

        //fprintf(stderr, "\t\tgetting maps lock\n");
        pthread_mutex_lock(&maps->m);
        get_next_rego(&maps->inside, &maps->outside, rand_m, &rego, &valid_rego);
        pthread_mutex_unlock(&maps->m);
        //fprintf(stderr, "\t\tlock released\n");
         
        pthread_mutex_lock(&entr_q[entrance].m);
        push(&entr_q[entrance].q, rego);
        pthread_cond_signal(&entr_q[entrance].c);
        pthread_mutex_unlock(&entr_q[entrance].m);

    }
}

void * entrance_queue(void * _args){
    entr_args_t* args = _args;
    volatile void * shm = args->shm;
    size_t entrance = args->entrance;
    shared_queue_t* entr_q = args->entrance_queue; 
    shared_queue_t* exit_qs = args->exit_queues;
    maps_t* maps = args->maps;
    Map* allow_list = args->allow_list;
    pthread_mutex_t* rand_m = args->rand_m;
    pthread_mutex_t* outer_level_m = args->outer_level_m;
    char * rego;

    struct timespec wait_time;
   
    // wait on the entrance lpr signal so we know the manager is ready
    pthread_mutex_lock(&ENTRANCE_LPR(entrance, shm)->m);
    pthread_cond_wait(&ENTRANCE_LPR(entrance, shm)->c, &ENTRANCE_LPR(entrance, shm)->m); 
    pthread_mutex_unlock(&ENTRANCE_LPR(entrance, shm)->m);
    
    while(1){
        pthread_mutex_lock(&entr_q->m);
        rego = pop(&entr_q->q); 

        if(rego == NULL){
            fprintf(stderr, "queue empty\n");
            // wait for a signal that there is another rego in the queue
//            fprintf(stderr, "sleep\n");
            pthread_mutex_unlock(&entr_q->m);
            usleep(1*1000);
            continue;
        }
        pthread_mutex_unlock(&entr_q->m);

        // car has reached the front of the queue
        // wait 2 ms as per the spec
        SLEEP(2);

        
        pthread_mutex_lock(&ENTRANCE_LPR(entrance, shm)->m);  
        pthread_mutex_lock(&ENTRANCE_BOOM(entrance, shm)->m);
        pthread_mutex_lock(&ENTRANCE_SIGN(entrance, shm)->m);
        
        // copy the rego into the lpr
        memcpy(&ENTRANCE_LPR(entrance, shm)->rego, rego, 6);

        // signal the lpr
        pthread_cond_signal(&ENTRANCE_LPR(entrance, shm)->c);

        // unlock the lpr so the entrance thread can take the lock 
        pthread_mutex_unlock(&ENTRANCE_LPR(entrance, shm)->m);

        // wait for the entrance thread to write a response on the sign
        pthread_cond_wait(&ENTRANCE_SIGN(entrance, shm)->c, &ENTRANCE_SIGN(entrance, shm)->m);
         
        fprintf(stderr, "reading sign\n");
        // read the sign
        char val = ENTRANCE_SIGN(entrance, shm)->display;

        // the car is allowed in val is between 1 and 5 in ascii 
        // encoding
        if( val >= '1' && val <= '5' ){
            // val is between 1 and 5 in ascii encoding
            fprintf(stderr, "waiting on boom\n");
            pthread_cond_wait(&ENTRANCE_BOOM(entrance, shm)->c, &ENTRANCE_BOOM(entrance, shm)->m);
            
            char boom_val = ENTRANCE_BOOM(entrance, shm)->state; 

            if(boom_val == 'L'){

                fprintf(stderr, "boom lowering\n");
                SLEEP(10); 
                ENTRANCE_BOOM(entrance, shm)->state = 'C'; 
                pthread_cond_signal(&ENTRANCE_BOOM(entrance, shm)->c);
                pthread_cond_wait(&ENTRANCE_BOOM(entrance, shm)->c, &ENTRANCE_BOOM(entrance, shm)->m);
                boom_val = ENTRANCE_BOOM(entrance, shm)->state;
                assert(boom_val == 'R');
            }
            
            if(boom_val == 'R'){
                fprintf(stderr, "boom raising\n");
                SLEEP(10); 
                ENTRANCE_BOOM(entrance, shm)->state = 'O'; 
                pthread_cond_signal(&ENTRANCE_BOOM(entrance, shm)->c);
            }

            int level = val - 48;

            // start the car thread
            car_args_t * args = calloc(1, sizeof(car_args_t));
            args->level = level; 
            args->shm = shm;
            args->rego = rego;
            args->exit_queues = exit_qs;
            args->rand_m = rand_m;
            args->outer_level_m = outer_level_m;
            pthread_t tmp; 
            pthread_create(&tmp, NULL, &car, args); 
        }
        // car isn't allowed in
        else{

            fprintf(stderr, "denied\n");
            // check if this is a car on the allow list, if it is we need
            // to insert it back into the outside map
            if(exists(allow_list, rego)){
                pthread_mutex_lock(&maps->m);     
                insert(&maps->outside, rego, NULL); 
                pthread_mutex_unlock(&maps->m);
                rego = NULL;
            }
            else{
                // this was a generated car so lets free the memory
                free(rego);
            }
        }
        
        pthread_mutex_unlock(&ENTRANCE_BOOM(entrance, shm)->m);
        pthread_mutex_unlock(&ENTRANCE_SIGN(entrance, shm)->m);
    }
}

void * exit_thr(void * _args){
    exit_args_t * args = _args;
    size_t  exit = args->exit;
    volatile void * shm = args->shm;
    Map * allow_list = args->allow_list;
    shared_queue_t* exit_q = args->exit_queue;
    maps_t* maps = args->maps;
    char * rego;

    while(1){
        pthread_mutex_lock(&exit_q->m);
        rego = pop(&exit_q->q); 
        
        if(rego == NULL){
            pthread_mutex_unlock(&exit_q->m);
            usleep(1*1000);
            continue;
            // wait for a signal that there is another rego in the queue
            /*
            pthread_cond_wait(&exit_q->c, &exit_q->m);
            pthread_mutex_unlock(&exit_q->m);
            continue;
            */
        }
        pthread_mutex_unlock(&exit_q->m);
        
        pthread_mutex_lock(&EXIT_LPR(exit, shm)->m);
        pthread_mutex_lock(&EXIT_BOOM(exit, shm)->m);
        
        // write rego into lpr
        memcpy(&EXIT_LPR(exit, shm)->rego, rego, 6);
       
        pthread_cond_signal(&EXIT_LPR(exit, shm)->c);
        pthread_mutex_unlock(&EXIT_LPR(exit, shm)->m);

        pthread_cond_wait(&EXIT_BOOM(exit, shm)->c, &EXIT_BOOM(exit, shm)->m);

        char boom_val = EXIT_BOOM(exit, shm)->state; 

        if(boom_val == 'L'){
            SLEEP(10); 
            EXIT_BOOM(exit, shm)->state = 'C'; 
            pthread_cond_signal(&EXIT_BOOM(exit, shm)->c);
            pthread_cond_wait(&EXIT_BOOM(exit, shm)->c, &EXIT_BOOM(exit, shm)->m);
            boom_val = EXIT_BOOM(exit, shm)->state;
            assert(boom_val == 'R');
        }
        
        if(boom_val == 'R'){
            SLEEP(10); 
            EXIT_BOOM(exit, shm)->state = 'O'; 
            pthread_cond_signal(&EXIT_BOOM(exit, shm)->c);
        }

        pthread_mutex_unlock(&EXIT_BOOM(exit, shm)->m);
        // EXIT_LPR mutex has already been unlocked

        // if the rego was on the allow list add it back to outside
        // and remove it from inside
        if(exists(allow_list, rego)){

            fprintf(stderr, "waiting on lock\n");
            pthread_mutex_lock(&maps->m);
            insert(&maps->outside, rego, NULL); 
            remove_key(&maps->inside, rego);
            fprintf(stderr, "done with lock\n");
            pthread_mutex_unlock(&maps->m);
            rego = NULL;
        }
        else{
            // rego was generated, we need to free the memory here
            free(rego);
        }
    }
}

void * car(void * _args){
    car_args_t* args = _args;
    int level = args->level;
    volatile void * shm = args->shm;
    shared_queue_t* exit_qs = args->exit_queues;
    char * rego = args->rego;
    pthread_mutex_t* rand_m = args->rand_m;
    pthread_mutex_t* outer_level_m = args->outer_level_m;
  
    // 
    // car takes 10 milliseconds to get to the level
    SLEEP(10);

    // get outerlevel lock for level
    pthread_mutex_lock(&outer_level_m[level-1]);

    // get lpr lock
    pthread_mutex_lock(&LEVEL_LPR(level, shm)->m);
    
    // write rego to level
    memcpy(&LEVEL_LPR(level, shm)->rego, rego, 6);
    
    pthread_cond_signal(&LEVEL_LPR(level, shm)->c);

    // wait for the level to signal that everything is done
    pthread_cond_wait(&LEVEL_LPR(level, shm)->c, &LEVEL_LPR(level, shm)->m);
    pthread_mutex_unlock(&LEVEL_LPR(level, shm)->m);
    
    // release outerlevel lock
    pthread_mutex_unlock(&outer_level_m[level-1]);

    // generate a random time from 
    pthread_mutex_lock(rand_m);
    // get a random int between 100-10000
    int park_time = (rand() % 9901) + 100;
    pthread_mutex_unlock(rand_m);

    // park for park_time milliseconds
    //usleep(2*1000);
    SLEEP(park_time);

    // get outerlevel lock for level
    pthread_mutex_lock(&outer_level_m[level-1]);

    // get lpr lock
    pthread_mutex_lock(&LEVEL_LPR(level, shm)->m);
    
    // write rego to level
    memcpy(&LEVEL_LPR(level, shm)->rego, rego, 6);
    
    pthread_cond_signal(&LEVEL_LPR(level, shm)->c);

    // wait for the level to signal that everything is done
    pthread_cond_wait(&LEVEL_LPR(level, shm)->c, &LEVEL_LPR(level, shm)->m);
    pthread_mutex_unlock(&LEVEL_LPR(level, shm)->m);
    
    // release outerlevel lock
    pthread_mutex_unlock(&outer_level_m[level-1]);
    
    // it takes 10 milliseconds to get to a random exit
    SLEEP(10);

    // go to a random exit
    pthread_mutex_lock(rand_m);
    int exit = (rand() % EXITS);
    pthread_mutex_unlock(rand_m);
    
    pthread_mutex_lock(&exit_qs[exit].m);
    push(&exit_qs[exit].q, rego);

    pthread_cond_signal(&exit_qs[exit].c);
    pthread_mutex_unlock(&exit_qs[exit].m);

    // free our arguments
    free(args);
}

// TODO: make it more likely to increase given that its decreased
void * temp_setter(void * _args){
    temp_args_t * args = _args;
    volatile void * shm = args->shm;
    pthread_mutex_t* rand_m = args->rand_m;
    int sleep_time = 1;
    
    // 0 is decrease
    // 1 is increase
    int scale[LEVELS] = {0, 0, 0, 0, 0};

    int change = 0;
    short change_val = 0;
  
    // lets just say the valid range is between 10 and 40 
    pthread_mutex_lock(rand_m);
    short start = (rand() % 31 + 10); 
    pthread_mutex_unlock(rand_m);
    
    // set the start temperatures
    for(int i = 1; i<=LEVELS; i++){
        *LEVEL_TEMP(i, shm) = start; 
    }

    while(1){
        SLEEP(sleep_time); 
        // increase, decrease or keep the temp the same on
        // each level
        for(int i=1; i<=LEVELS; i++){
            pthread_mutex_lock(rand_m); 
            sleep_time = (rand() % 5) +1;
            change = rand() % 100;
            change_val = rand() % 30000; 
            pthread_mutex_unlock(rand_m);
            
            if(change <= 80){
                continue;
            }

            // 51% of the time
            if(change_val <= 15000){
                change_val = 1;
            }
            // 25% of the time
            else if(change_val > 15000 && change_val <= 20000){
                change_val = 2;
            }
            // 12% of the time
            else if(change_val > 20000 && change_val <= 25000){
                change_val = 3; 
            }
            // 6% of the time
            else if(change_val > 25000 && change_val <= 29997){
                change_val = 4; 
            }

            else if(change_val > 29997 && change_val <= 29998){
                
                // fixed temp
                short fixed_val = 65;

                for (size_t a = 0; a < 30; a++){
                    *LEVEL_TEMP(i,shm) = fixed_val; 
                } 
                change_val = 1;
            }

            else if(change_val > 29998 && change_val <= 30000){

                // rate of rise
                short rate_val = 1;

                for (size_t a = 0; a < 30; a++){
                    *LEVEL_TEMP(i,shm) += rate_val; 
                    rate_val ++;
                }
                change_val = 1;
            }

            int middle = 10;
            middle += scale[i-1];
            // 12 % chance of decrease
            if(change > 80 && change <= 80+middle ){
                scale[i-1] -= change_val; 
                *LEVEL_TEMP(i,shm) -= change_val;   
            }
            // 12 % chance of  increase
            else{

                scale[i-1] += change_val; 
                *LEVEL_TEMP(i,shm) += change_val;   
            }
        }
    }
}
