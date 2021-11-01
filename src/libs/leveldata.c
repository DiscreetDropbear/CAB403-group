#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../include/types.h"
#include "../include/leveldata.h"
#include "../include/map.h"

level_t* init_level(size_t level_cap);
void free_level(level_t* level);

void init_level_data(level_data_t* ld){
    ld->num_levels = LEVELS;
    ld->levels = calloc(LEVELS, sizeof(level_t*));

    for(int i = 0; i < LEVELS; i++){
        ld->levels[i] = init_level(LEVEL_CAPACITY); // defined 
    }
}

void free_level_data(level_data_t* ld){
    for(int i = 0; i < ld->num_levels; i++){
        free_level(ld->levels[i]);
    }
}

// returns a level that has the most free parks
size_t get_available_level(level_data_t* ld){

    int largest = LEVEL_CAPACITY - cars_in_level(ld, 1);
    int index = 1;

    for(int i = 1; i <= LEVELS; i++){
        // if there are any available spots  
        if(LEVEL_CAPACITY - cars_in_level(ld, i) > largest){  
            largest = LEVEL_CAPACITY - cars_in_level(ld, i);
            index = i; 
        }
    }
   
    if(LEVEL_CAPACITY - cars_in_level(ld, index) == 0){
        // 0 means here are no avilable parks on any level
        return 0;
    }
    else{
        return index;
    }
}

// tries to insert the key:value pair in the given level, returns true if successfull
// or false otherwise
// rego is copied and then inserted into the map so the callers copy will still be
// valid and they must free it
bool insert_in_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);
    if(LEVEL_CAPACITY - cars_in_level(ld, l_num) > 0){

        char* regoc = malloc(7);
        memcpy((void*)regoc, rego, 7);
        // just using the void* as a bool value instead of a pointer
        res_t res = insert(&ld->levels[l_num-1]->cars, regoc, NULL);
        // there shouldn't be any case where we are inserting a rego into a level and 
        // there is already the same rego there
        assert(res.exists == false); 
        return true;
    }
    return false;
}

// searches for the given rego in the level specified by l_num
// returns a res_t which specifies if the rego was found and what the value 
// is if it was found
res_t search_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);

    // since the value pointer in res_t is actually encoding a boolean we
    // can just return the res_t since we know it won't be dereferenced
    return search(&ld->levels[l_num-1]->cars, rego);    
}

// removes a rego from a given level incrementing the free_parks for that level 
// returns a res_t which specifies if the rego was in the level and what the value 
// is if it was found
res_t remove_from_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);

    res_t res = remove_key(&ld->levels[l_num-1]->cars, rego); 
    
    return res;
}

res_t remove_from_all_levels(level_data_t* ld, char* rego){
    int removed = 0;
    for(int i = 0; i<ld->num_levels; i++){
        if(LEVEL_CAPACITY - LEVEL_CAPACITY - cars_in_level(ld, i+1) > 0){
            res_t res = remove_from_level(ld, i+1, rego);
            if(res.exists){
                removed++;
            }
        }
    }

    assert(removed == 1);
}

// returns true if the given rego exists in the level referred to by l_num 
bool exists_in_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);
    return exists(&ld->levels[l_num-1]->cars, rego);
}

// returns the number of free parks in the level referred to by l_num
size_t cars_in_level(level_data_t* ld, size_t l_num){
    assert(l_num <= ld->num_levels);
    return get_count(&ld->levels[l_num-1]->cars);
}

//
/// THE FOLLOWING FUNCTIONS ARE NOT A PART OF THE PUBLIC API
//

void free_level(level_t * level){
    free_map(&level->cars);
    free(level);
}

// allocates a new level, initializes it and returns a pointer to it
level_t* init_level(size_t level_cap){
    level_t * l = calloc(1, sizeof(level_t));
    if(l == NULL){
        fprintf(stderr, "error allocating memory for level_t in leveldata.c init_level();");
        abort();
    }

    init_map(&l->cars, level_cap);

    return l;
}
