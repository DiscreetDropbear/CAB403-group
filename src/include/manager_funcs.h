#ifndef MANAGER_FUNCS_H
#define MANAGER_FUNCS_H

typedef struct entrance_args{
    int thread_num;
    volatile void * shared_mem;
    Map* allow_list;
    pthread_mutex_t* billing_m;
    billing_t* billing;
    // level_m protexts billing and free_spots
    pthread_mutex_t* level_m;
    level_data_t* level_d;
    size_t* free_spots; 
} entrance_args_t;

typedef struct level_args{
    int thread_num;
    volatile void * shared_mem;
    pthread_mutex_t* level_m;
    level_data_t* level_d;
} level_args_t;

typedef struct exit_args{
    int thread_num;
    volatile void * shared_mem;
    pthread_mutex_t* billing_m;
    billing_t* billing;
    // level_m protects billing and free_spots
    pthread_mutex_t* level_m;
    level_data_t* level_d;
    size_t* free_spots; 
} exit_args_t;

void* entrance_thread(void *);
void* exit_thr(void*);
void* level_thr(void*);

#endif
