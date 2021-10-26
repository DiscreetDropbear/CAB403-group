#ifndef SIMULATOR_FUNCS_H
#define SIMULATOR_FUNCS_H
#include <pthread.h> 
#include "queue.h"

typedef struct shared_queue{
    pthread_mutex_t m;
    pthread_cond_t c;
    Queue q;
} shared_queue_t;

typedef struct spawner_args{
    shared_queue_t* entrance_queues;    
    shared_queue_t* exit_queues;
} spawner_args_t;

void * spawner(void *);
void * entrance_queue(void *);
void * car(void *);
void * exit_thr(void *);
void * temp_setter(void *);

#endif
