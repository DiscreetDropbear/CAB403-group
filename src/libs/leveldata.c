#include "types.h"
#include "leveldata.h"
#include "map.h"
#include <stdlib.h>
#include <stdio.h>

void init_level(level_t* level);
void free_level(level_t * level);

void init_level_data(level_data_t* ld, size_t num_levels){
    ld->num_levels = num_levels;
    for(int i = 0; i < num_levels; i++){
        ld->levels[i] = init_level();
    }
}

void free_level_data(level_data_t* ld){
    for(int i = 0; i < ld->num_levels; i++){
        free_level(ld->levels[i]);
    }
}

// returns the first available level that has free parks 
size_t get_available_level(level_data_t* ld){
    for(int i = 0; i < ld->num_levels; i++){
        // if there are any available spots  
        if(ld->levels[i]->free_parks > 0){
            return i+1;
        }
    }
    
    // 0 means here are no avilable parks on any level
    return 0;
}

// tries to insert the key:value pair in the given level, returns true if successfull
// or false otherwise
// rego is copied and then inserted into the map so the callers copy will still be
// valid and they must free it
bool insert_in_level(level_data_t* ld, size_t l_num, char* rego, bool val){
    assert(l_num <= ld->num_levels);
    if(ld->levels[l_num]->free_parks > 0){
        char* regoc = malloc(7);
        memcpy((void*)regoc, rego, 7);
        // just using the void* as a bool value instead of a pointer
        res_t res = insert(ld->levels[l_num-1]->cars, regoc, (void*)val);
        // there shouldn't be any case where we are inserting a rego into a level and 
        // there is already the same rego there
        assert(res.exists == false); 
        ld->levels[l_num-1]->free_parks--;
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
    return search(ld->levels[l_num-1]->cars, rego);    
}

// removes a rego from a given level incrementing the free_parks for that level 
// returns a res_t which specifies if the rego was in the level and what the value 
// is if it was found
res_t remove_from_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);

    res_t res = remove_key(ld->levels[l_num-1]->cars, rego); 

    if(res.exists){
        ld->levels[l_num-1]->free_parks++;
    }

    return res;
}

// returns true if the given rego exists in the level referred to by l_num 
bool exists_in_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);
    return exists(ld->levels[l_num-1], rego);
}

// returns the number of free parks in the level referred to by l_num
size_t levels_free_parks(level_data_t* ld, size_t l_num){
    assert(l_num <= ld->num_levels);
    return ld->levels[l_num-1]->free_parks;
}

//
/// THE FOLLOWING FUNCTIONS ARE NOT A PART OF THE PUBLIC API
//

void free_level(level_t * level){
    free_map(level->cars);
    free(level);
}

// allocates a new level, initializes it and returns a pointer to it
level_t* init_level(size_t level_cap){
    level_t * l = calloc(1, sizeof(level_t));
    if(l == NULL){
        fprintf(stderr, "error allocating memory for level_t in leveldata.c init_level();");
        abort(-1);
    }

    l->free_parks = level_cap;
    init_map(&l->map, level_cap);

    return l;
}
