#include "types.h"
#include "leveldata.h"
#include "map.h"
#include <stdlib.h>
#include <stdio.h>

void init_level(level_t* level);

void init_level_data(level_data_t* ld, size_t num_levels){
    ld->num_levels = num_levels;
    for(int i = 0; i < num_levels; i++){
        ld->levels[i] = init_level();
    }
}

void free_level_data(level_data_t* ld){
    // TODO:
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
bool insert_in_level(level_data_t* ld, size_t l_num, char* rego, bool val){
    assert(l_num <= ld->num_levels);
    if(ld->levels[l_num]->free_parks > 0){
        insert(ld->levels[l_num]->cars, rego, (void*)val);
        ld->levels[l_num]->free_parks--;
        return true;
    }
    
    return false;
}

// searches for the given rego in the level specified by l_num
// returns a res_t which specifies if the rego was found and what the value 
// is if it was found
bool search_in_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);
    
}

// removes a rego from a given level incrementing the free_parks for that level 
// returns a res_t which specifies if the rego was in the level and what the value 
// is if it was found
res_t remove_from_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);


}

// returns true if the given rego exists in the level referred to by l_num 
bool exists_in_level(level_data_t* ld, size_t l_num, char* rego){
    assert(l_num <= ld->num_levels);
}
// returns the number of free parks in the level referred to by l_num
size_t levels_free_parks(level_data_t* ld, size_t l_num){
    assert(l_num <= ld->num_levels);

}

//
/// THE FOLLOWING FUNCTIONS ARE NOT A PART OF THE PUBLIC API
//

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
