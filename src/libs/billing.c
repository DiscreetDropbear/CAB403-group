#include <time.h>
#include <assert.h>
#include <stdio.h>
#include "map.h"

int init_billing(Billing* billing){
    assert(billing != NULL);
   
    init_map(&billing->map);
    return 0;
}

void* insert_rego(Billing* billing, char * rego){
    assert(billing != NULL);
    // copy the rego
    char * regoc = malloc(7);
    memcpy((void*)regoc, (void*)rego, 7);

    struct timespec* time = calloc(sizeof(struct timespec));
    struct timespec* res;
	clock_gettime(CLOCK_MONOTONIC, time);

    res = insert(&billing->map, regoc, (void*)time);
}

// returns the timespec for a given rego removing it from the 
// set of regos in the map, its the callers job to free the memory both
// for the timespec and 
timespec * remove_rego(Billing* billing, char * rego){
    assert(billing != NULL);
    return remove_key(&billing->map, rego);
}
