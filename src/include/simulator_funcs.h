#ifndef SIMULATOR_FUNCS_H
#define SIMULATOR_FUNCS_H
#include <pthread.h> 
#include "queue.h"
#include "types.h"
#include "map.h"

typedef struct shared_queue{
    pthread_mutex_t m;
    pthread_cond_t c;
    Queue q;
} shared_queue_t;

typedef struct maps{
    pthread_mutex_t m;   
    Map inside;
    Map outside;
} maps_t;

typedef struct generator_args{
    
    shared_queue_t* entrance_queues;    
    shared_queue_t* exit_queues;
    maps_t* maps; 
    pthread_mutex_t* rand_m;
} generator_args_t;

typedef struct entr_args{
    size_t entrance;
    volatile void * shm;
    shared_queue_t* entrance_queue;    
    shared_queue_t* exit_queues;
    maps_t* maps; 
    Map* allow_list;
    pthread_mutex_t* rand_m;
    pthread_mutex_t* outer_level_m;
} entr_args_t;

typedef struct exit_args{
    size_t exit;
    volatile void * shm;
    Map* allow_list;
    shared_queue_t* exit_queue;
    maps_t* maps; 
} exit_args_t;

typedef struct car_args{
    int level;
    volatile void* shm;
    char * rego;
    shared_queue_t* exit_queues;
    pthread_mutex_t* rand_m;
    pthread_mutex_t* outer_level_m;
} car_args_t;

typedef struct temp_args{
    volatile void* shm;
    pthread_mutex_t* rand_m;
} temp_args_t;

void * generator(void *);
void * entrance_queue(void *);
void * car(void *);
void * exit_thr(void *);
void * temp_setter(void *);
void* boom_thread(void * args);

#endif
