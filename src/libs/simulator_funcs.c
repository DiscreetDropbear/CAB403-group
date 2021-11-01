#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
        rego[i] = (char)(rand() %(57 - 48 + 1) + 48); //char range for numbers
    }
    // Generate three random numbers
 	for (int i = 3; i < 6; i++) {
        rego[i] = (char)(rand() %(90 - 65 + 1) + 65); //char range for letter 
    }
    pthread_mutex_unlock(rand_m);

    rego[6] = '\0';
    
    return 0;
}

// returns a random rego from the allow list
void select_valid_rego(char ** regos, int num_regos, char * out, pthread_mutex_t* rand_m){

    pthread_mutex_lock(rand_m);
    int i = rand() % num_regos;
    pthread_mutex_unlock(rand_m);
    
    memcpy(out, regos[i], 6);
}

// returns 0 if the rego is from the allow list or 1 if it was generated
int get_next_rego(maps_t * maps, pthread_mutex_t* rand_m, char ** rego){
    pthread_mutex_lock(rand_m);
    int generate = (rand() % 10);
    pthread_mutex_unlock(rand_m);

    *rego = calloc(7, sizeof(char));

    if(generate > 2 || ONLY_ALLOWED){
        // loop to make sure when ONLY_ALLOWED is set we dont go
        // further to generate a random one
        do{
            pthread_mutex_lock(&maps->m);
            pthread_mutex_lock(rand_m);
            pair_t res = get_random_item(&maps->outside);
            pthread_mutex_unlock(rand_m);
            pthread_mutex_unlock(&maps->m);
            
            if(res.key != NULL){
                *rego = res.key;
                pthread_mutex_lock(&maps->m);
                insert(&maps->inside, *rego, NULL); 
                pthread_mutex_unlock(&maps->m);
                return 0;
            }
        }
        while(ONLY_ALLOWED);
            // the outside map must be empty
            // generate a new one
    }
        
    // either generate is 1 or there are no more valid regos left
    // so we will generate a random rego
    // we need to setup the rego's memory
    while(1){
        generate_rego(*rego, rand_m);
        pthread_mutex_lock(&maps->m);
        bool condition = !exists(&maps->inside, *rego) && !exists(&maps->outside, *rego); 
        pthread_mutex_unlock(&maps->m);
        // make sure the rego doesn't already exist in the outside map(the allowed cars)
        // or the inside map(the cars that are already inside the carpark)
        if(condition){
            pthread_mutex_lock(&maps->m);
            insert(&maps->inside, *rego, NULL);
            pthread_mutex_unlock(&maps->m);
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
    char* rego;

    int start = 0;
    while(1){
        
        pthread_mutex_lock(rand_m);
        //use rand to generate number between 1 and 100 inclusive to determine spawn rate
        int st = rand() % 100 + 1;         
        int entrance = rand() % ENTRANCES;
        pthread_mutex_unlock(rand_m);

        SLEEP(st);
        //usleep(2*1000);
        //fprintf(stderr, "got rego\n");

        get_next_rego(maps, rand_m, &rego);
         
        pthread_mutex_lock(&entr_q[entrance].m);
        push(&entr_q[entrance].q, rego);
        pthread_cond_signal(&entr_q[entrance].c);
        pthread_mutex_unlock(&entr_q[entrance].m);
    }
}

void * entrance_queue(void * _args){
    entr_args_t* args = _args;
    volatile void * shm = args->shm;
    const size_t entrance = args->entrance;
    shared_queue_t* entr_q = args->entrance_queue; 
    shared_queue_t* exit_qs = args->exit_queues;
    Map* allow_list = args->allow_list;
    maps_t* maps = args->maps;
    pthread_mutex_t* rand_m = args->rand_m;
    pthread_mutex_t* outer_level_m = args->outer_level_m;
    char * rego;

    int count = 0;
    struct timespec wait_time;
   
    // wait on the entrance lpr signal so we know the manager is ready
    pthread_mutex_lock(&ENTRANCE_LPR(entrance, shm)->m);
    pthread_cond_wait(&ENTRANCE_LPR(entrance, shm)->c, &ENTRANCE_LPR(entrance, shm)->m); 
    pthread_mutex_unlock(&ENTRANCE_LPR(entrance, shm)->m);
    
    while(1){
        pthread_mutex_lock(&entr_q->m);
        rego = pop(&entr_q->q); 

        if(rego == NULL){
            // wait for a signal that there is another rego in the queue
            pthread_mutex_unlock(&entr_q->m);
            usleep(1*1000);
            continue;
        }
        pthread_mutex_unlock(&entr_q->m);

        // car has reached the front of the queue
        // wait 2 ms as per the spec
        SLEEP(2);
        
        pthread_mutex_lock(&ENTRANCE_LPR(entrance, shm)->m);  
        pthread_mutex_lock(&ENTRANCE_SIGN(entrance, shm)->m);
        pthread_mutex_lock(&ENTRANCE_BOOM(entrance, shm)->m);

        // copy the rego into the lpr
        memcpy(&ENTRANCE_LPR(entrance, shm)->rego, rego, 6);

        // signal the lpr
        pthread_cond_signal(&ENTRANCE_LPR(entrance, shm)->c);

        // unlock the lpr so the entrance thread can take the lock 
        pthread_mutex_unlock(&ENTRANCE_LPR(entrance, shm)->m);

        // wait for the entrance thread to write a response on the sign
        pthread_cond_wait(&ENTRANCE_SIGN(entrance, shm)->c, &ENTRANCE_SIGN(entrance, shm)->m);
        char val = ENTRANCE_SIGN(entrance, shm)->display;
        pthread_cond_signal(&ENTRANCE_SIGN(entrance, shm)->c);
        pthread_mutex_unlock(&ENTRANCE_SIGN(entrance, shm)->m);

        //fprintf(stderr, "got sign value %c\n");

        // the car is allowed in val is between 1 and 5 in ascii 
        // encoding
        if( val >= '1' && val <= '5' ){
            
            //fprintf(stderr, "entered - %s - %c - %d\n", rego, val, count);
            count++;

            pthread_cond_wait(&ENTRANCE_BOOM(entrance, shm)->c, &ENTRANCE_BOOM(entrance, shm)->m);
            if(ENTRANCE_BOOM(entrance, shm)->state == 'L'){
                SLEEP(10);
                ENTRANCE_BOOM(entrance, shm)->state = 'C';
                pthread_cond_signal(&ENTRANCE_BOOM(entrance, shm)->c);
                pthread_cond_wait(&ENTRANCE_BOOM(entrance, shm)->c, &ENTRANCE_BOOM(entrance, shm)->m);
            }

            if(ENTRANCE_BOOM(entrance, shm)->state == 'R'){
                SLEEP(10);
                ENTRANCE_BOOM(entrance, shm)->state = 'O';
                pthread_cond_signal(&ENTRANCE_BOOM(entrance, shm)->c);
            }
            pthread_mutex_unlock(&ENTRANCE_BOOM(entrance, shm)->m);            

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

            int res; 
            do{ 
                res = pthread_create(&tmp, NULL, &car, args); 
                if(res != 0){
                    fprintf(stderr, "pthread_create error = %d\n", res);
                }
            }
            while(res != 0);
        }
        // car isn't allowed in
        else{


            pthread_mutex_unlock(&ENTRANCE_BOOM(entrance, shm)->m);            
            pthread_mutex_lock(&maps->m);     
            // if its on the allow list add it back to outside
            // to be picked again
            if(exists(allow_list, rego)){
                insert(&maps->outside, rego, NULL);
            }
            // its no longer in the queue so we can remove it
            // from inside
            remove_key(&maps->inside, rego);
            free(rego);
            pthread_mutex_unlock(&maps->m);
        }
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

    int num = 0;

    int count = 0;

    while(1){
        pthread_mutex_lock(&exit_q->m);
        rego = pop(&exit_q->q); 
        
        if(rego == NULL){
            /*
            pthread_mutex_unlock(&exit_q->m);
            usleep(1*1000);
            continue;
            */
            pthread_cond_wait(&exit_q->c, &exit_q->m);
            pthread_mutex_unlock(&exit_q->m);
            continue;
        }
        pthread_mutex_unlock(&exit_q->m);

        //fprintf(stderr, "exited - %s\n", rego);
        
        pthread_mutex_lock(&EXIT_LPR(exit, shm)->m);
        pthread_mutex_lock(&EXIT_BOOM(exit, shm)->m);

        //fprintf(stderr, "got lpr lock - %s\n", rego);
        // write rego into lpr
        memcpy(&EXIT_LPR(exit, shm)->rego, rego, 6);
       
        pthread_cond_signal(&EXIT_LPR(exit, shm)->c);
        pthread_cond_wait(&EXIT_LPR(exit, shm)->c, &EXIT_LPR(exit, shm)->m);
        //fprintf(stderr, "got lpr signal - %s\n", rego);
        pthread_cond_signal(&EXIT_LPR(exit, shm)->c);
        pthread_mutex_unlock(&EXIT_LPR(exit, shm)->m);

        //fprintf(stderr, "wait for boom lock- %s\n", rego);
        //fprintf(stderr, "boom - %s\n", rego);

        pthread_cond_wait(&EXIT_BOOM(exit, shm)->c, &EXIT_BOOM(exit, shm)->m);

        if(EXIT_BOOM(exit, shm)->state == 'L'){
            SLEEP(10);
            EXIT_BOOM(exit, shm)->state = 'C';
            pthread_cond_signal(&EXIT_BOOM(exit, shm)->c);
            pthread_cond_wait(&EXIT_BOOM(exit, shm)->c, &EXIT_BOOM(exit, shm)->m);
        }

        if(EXIT_BOOM(exit, shm)->state == 'R'){
            SLEEP(10);
            EXIT_BOOM(exit, shm)->state = 'O';
            pthread_cond_signal(&EXIT_BOOM(exit, shm)->c);
        }
        pthread_mutex_unlock(&EXIT_BOOM(exit, shm)->m);            

        count++;
        
        pthread_mutex_lock(&maps->m);
        // if its inside the car park we know that its allowed in
        // so insert it back into ouside
        insert(&maps->outside, rego, NULL);
        remove_key(&maps->inside, rego);
        free(rego);
        pthread_mutex_unlock(&maps->m);
    }
}

void visit_level(int level, volatile void* shm, char * rego, pthread_mutex_t* outer_level_m, pthread_mutex_t* rand_m){

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
    //usleep(park_time * 1000);
    usleep(2* 1000);
    //SLEEP(park_time);

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
}



void * car(void * _args){
    car_args_t* args = _args;
    int level = args->level;
    volatile void * shm = args->shm;
    shared_queue_t* exit_qs = args->exit_queues;
    char * rego = args->rego;
    pthread_mutex_t* rand_m = args->rand_m;
    pthread_mutex_t* outer_level_m = args->outer_level_m;
  
    bool visit_assigned = true;
    bool visited_assigned = false;
    int levels_visited = 0;
    int exit_decision;
    int num; 
    int random_level;

    while(1){
        pthread_mutex_lock(rand_m);
        exit_decision = rand() % 10+1; 
        pthread_mutex_unlock(rand_m);

        if(exit_decision + 2*levels_visited > 7){
            fprintf(stderr, "exiting car park - visited %d levels\n", levels_visited);
            //exit
            break; 
        }

        // go to a level 
        pthread_mutex_lock(rand_m);      
        num = rand() % 2; 
        pthread_mutex_unlock(rand_m);

        if(visit_assigned && num == 1){
            visit_level(level, shm, rego, outer_level_m, rand_m);  
            visited_assigned = true;
        }
        else{
            pthread_mutex_lock(rand_m);      
            random_level = rand() % LEVELS +1;
            pthread_mutex_unlock(rand_m);

            visit_level(random_level, shm, rego, outer_level_m, rand_m);
        }


        visit_assigned = false;
        levels_visited++;
    }

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


void* boom_thread(void * args){
    struct boom_t* boom = args; 
    return NULL;
    pthread_mutex_lock(&boom->m);
    while(1){
        pthread_cond_wait(&boom->c, &boom->m);

        if(boom->state == 'L'){
            SLEEP(10);
            boom->state = 'C';
            while(boom->state != 'O'){
                pthread_cond_broadcast(&boom->c);
            }
        }
    }
}

void * temp_setter(void * _args){
    temp_args_t * args = _args;
    volatile void * shm = args->shm;
    pthread_mutex_t* rand_m = args->rand_m;
    int sleep_time = 1;
    
    // 0 is decrease
    // 1 is increase
    int scale[LEVELS];
    for(int i=0;i<LEVELS;i++){
        scale[i]=0;
    }

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

               // increase, decrease or keep the temp the same on
        // each level
        for(int i=1; i<=LEVELS; i++){
            pthread_mutex_lock(rand_m); 
            change = rand() % 100;
            change_val = rand() % 100; 
            pthread_mutex_unlock(rand_m);
            
            if(change <= 80){
                continue;
            }

            change_val = 1;
            // 51% of the time
            if(change_val <= 50){
                change_val = 1;
            }
            // 25% of the time
            else if(change_val > 50 && change_val <= 75){
                change_val = 2;
            }
            // 12% of the time
            else if(change_val > 75 && change_val <= 87){
                change_val = 3; 
            }
            // 6% of the time
            else if(change_val > 87 && change_val <= 93){
                change_val = 4; 
            }

            int middle = 10;
            middle += scale[i-1];
            if(change > 80 && change <= 80+middle ){
                scale[i-1] -= change_val; 
                *LEVEL_TEMP(i,shm) -= change_val;   
            }
            else{

                scale[i-1] += change_val; 
                *LEVEL_TEMP(i,shm) += change_val;   
            }
        }
        
        pthread_mutex_lock(rand_m); 
        sleep_time = (rand() % 5) +1;
        pthread_mutex_unlock(rand_m);
        SLEEP(sleep_time); 


    }
}
