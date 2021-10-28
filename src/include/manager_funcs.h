#ifndef MANAGER_FUNCS_H
#define MANAGER_FUNCS_H

typedef struct entrance_args{
    int thread_num;
    volatile void * shm;
    Map* allow_list;
    pthread_mutex_t* billing_m;
    billing_t* billing;
    pthread_mutex_t* level_m;
    level_data_t* level_d;
    size_t* free_spots; 
} entrance_args_t;

typedef struct level_args{
    int thread_num;
    volatile void * shm;
    pthread_mutex_t* level_m;
    level_data_t* level_d;
} level_args_t;

typedef struct exit_args{
    int thread_num;
    volatile void * shm;
    pthread_mutex_t* billing_m;
    billing_t* billing;
    FILE* billing_fp;
    unsigned int* total_bill; // current bill for the whole car park in cents
    pthread_mutex_t* level_m;
    level_data_t* level_d;
    size_t* free_spots; 
} exit_args_t;

typedef struct display_args{
    volatile void * shm;
    pthread_mutex_t* billing_m;
    unsigned int* bill; // current bill for the whole car park in cents
    // level_m protects billing and free_spots
    pthread_mutex_t* level_m;
    level_data_t* level_d;
    size_t* free_spots; 
} display_args_t;

void* entrance_thread(void *);
void* exit_thread(void*);
void* level_thread(void*);
void* display_thread(void*);
#endif
