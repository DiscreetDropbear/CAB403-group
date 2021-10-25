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
#include "include/manager_funcs.h"

void entrance_tests(entrance_args_t entrance_args);
void test_one(entrance_args_t entrance_args);
void test_two(entrance_args_t entrance_args);
void test_three(entrance_args_t entrance_args);

int main(){

    /// setup shared memory
	int shm_fd = shm_open("PARKING", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    if(shm_fd < 0){
        printf("failed opening shared memory with errno: %d\n", errno);
        return -1;
    }

    /// resize shared memory
    ftruncate(shm_fd, 2920);

    /// memory map the shared memory
	volatile void * shm = (volatile void *) mmap(0, 2920, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    /// setup the mutex's and condition variables for use between processes
    //
    init_shared_mem(shm);

    Map allow_list;
    pthread_mutex_t level_m;
    level_data_t level_d;
    size_t free_spots = LEVELS * LEVEL_CAPACITY;
    pthread_mutex_t billing_m;
    billing_t billing;
    
    init_map(&allow_list, 0);
    // setup the billing and billing mutex
    init_billing(&billing);
    pthread_mutex_init(&billing_m, NULL);
    init_level_data(&level_d, LEVELS, LEVEL_CAPACITY);
    pthread_mutex_init(&level_m, NULL);
    
    entrance_args_t entrance_args;
    level_args_t level_args;
    exit_args_t exit_args;

    // 
    entrance_args.thread_num = 1; 
    entrance_args.shared_mem = shm; 
    entrance_args.allow_list = &allow_list; 
    entrance_args.billing_m = &billing_m;
    entrance_args.billing = &billing;
    entrance_args.level_m = &level_m;
    entrance_args.level_d = &level_d;
    entrance_args.free_spots = &free_spots;

    exit_args.thread_num = 1; 
    exit_args.shared_mem = shm; 
    exit_args.billing_m = &billing_m;
    exit_args.billing = &billing;
    exit_args.level_m = &level_m;
    exit_args.level_d = &level_d;
    exit_args.free_spots = &free_spots;

    level_args.thread_num = 1;
    level_args.shared_mem = shm;
    level_args.level_m = &level_m;
    level_args.level_d = &level_d;

    // entrance tests
    entrance_tests(entrance_args);    
    // level tests

    // exit tests

    //TODO: close shared memory
    return 0;
}

//
/// ENTRANCE TESTS
//

void setup_entrance_vars(entrance_args_t entrance_args){
    init_map(entrance_args.allow_list, 0);
    // setup the billing and billing mutex
    init_billing(entrance_args.billing);
    init_level_data(entrance_args.level_d, LEVELS, LEVEL_CAPACITY);
}

void entrance_tests(entrance_args_t entrance_args){
    pthread_t t;
    volatile void* shm = entrance_args.shared_mem;

    // lock LPR just before launching thread so we can start the thread then
    // wait on the signal
    pthread_mutex_lock(&ENTRANCE_LPR(1, shm)->m);

    // start thread
    pthread_create(&t, NULL, &entrance_thread, &entrance_args);         

    // wait on the lpr signal to know when the entrance thread is ready,
    // NOTE: This unlocks the entrance lpr lock
    pthread_cond_wait(&ENTRANCE_LPR(1, shm)->c, &ENTRANCE_LPR(1, shm)->m); 

    test_one(entrance_args);

    setup_entrance_vars(entrance_args);
    test_two(entrance_args);

    setup_entrance_vars(entrance_args);
    test_three(entrance_args);

    //fprintf(stderr, "\nAAAAAAAA\n");
    // cancel thread
    pthread_cancel(t);
}

// test that adding a car thats in the allow list gets assigned a random number
// between 1 and 5, put into the billing map and the levels map
void test_one(entrance_args_t entrance_args){
    fprintf(stdout, "TEST ONE START\n");
    char* rego = malloc(sizeof(char)*7);
    memcpy(rego, "123ABC\0", 7);

    volatile void* shm = entrance_args.shared_mem;

    // insert into allow list, note the entrance thread should be waiting on the lock at the moment
    res_t res = insert(entrance_args.allow_list, rego, (void*)true); 

    // get all of the locks
    pthread_mutex_lock(&ENTRANCE_SIGN(1, shm)->m);
    pthread_mutex_lock(&ENTRANCE_BOOM(1, shm)->m);

    // copy the rego into the lpr
    memcpy(&ENTRANCE_LPR(1, shm)->rego, rego, 6);

    // signal the lpr
    pthread_cond_signal(&ENTRANCE_LPR(1, shm)->c);

    // unlock the lpr so the entrance thread can take the lock 
    // to wait on the signal next round of the while loop
    pthread_mutex_unlock(&ENTRANCE_LPR(1, shm)->m);
   
    // wait for the entrance thread to write a response on the sign
    pthread_cond_wait(&ENTRANCE_SIGN(1, shm)->c, &ENTRANCE_SIGN(1, shm)->m);
    
    // read the sign
    char val = ENTRANCE_SIGN(1, shm)->display;
    fprintf(stdout, "sign value = %c\n", val);
    // sign should display a number between 1 and 5
    assert(val >= 49 && val <= 53);
    
    pthread_cond_wait(&ENTRANCE_BOOM(1, shm)->c, &ENTRANCE_BOOM(1, shm)->m);
    
    char boom_val = ENTRANCE_BOOM(1, shm)->state; 
    fprintf(stdout, "boom value = %c\n", boom_val);
    assert(boom_val == 'R');

    // this is where we wait for 10ms
    ENTRANCE_BOOM(1, shm)->state = 'O'; 
    pthread_cond_signal(&ENTRANCE_BOOM(1, shm)->c);
    pthread_mutex_unlock(&ENTRANCE_BOOM(1, shm)->m);


    fprintf(stdout, "TEST ONE END\n");

}

void test_two(entrance_args_t entrance_args){
}

void test_three(entrance_args_t entrance_args){
}

//
// LEVEL TESTS
//

//
// EXIT TESTS
//
