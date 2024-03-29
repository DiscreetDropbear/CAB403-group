#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/billing.h"
#include "../include/utils.h"

void init_billing(billing_t* billing){
    assert(billing != NULL);
   
    init_map(&billing->map, 0);
}

void insert_rego(billing_t* billing, char * rego){
    assert(billing != NULL);
    assert(rego != NULL);
    // copy the rego
    struct timespec* time = calloc(1, sizeof(struct timespec));
	clock_gettime(CLOCK_MONOTONIC, time);

    res_t res = insert(&billing->map, rego, (void*)time);
    assert(res.exists == false);
}

// returns the number of milliseconds the car was inside the car park 
// removing the rego from the map
int remove_rego(billing_t* billing, char * rego, unsigned long* duration_ms){
    assert(billing != NULL);

    res_t res = remove_key(&billing->map, rego);
    if(res.exists == true){
        // calculate the time spent in the car park
        int r = time_diff(*(struct timespec*)res.value, duration_ms);        
        if(r != 0){
            fprintf(stderr, "time_diff error\n"); 
            return r;
        }
        return 0;
    }
    else{
        return -1; 
    }
}
