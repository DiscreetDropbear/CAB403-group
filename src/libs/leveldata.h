#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H
#include "map.h"

typedef struct level{
    Map cars; // map with regos for keys and bools for values 
    size_t free_parks;
} level_t;

typedef struct level_data{
    level ** levels; // array of levels
    size_t num_levels; // number of levels there are
} level_data_t

// result type for remove_from_level
// so we can tell if it was in the level or not
// and what the return value was if it was in the 
// level
typedef struct res{
    bool exists;
    bool value;
} res_t

void init_level_data(level_data_t * ld);
// returns the first available level that has free parks 
size_t get_available_level(level_data_t * ld);

res_t search_level(level_data_t* ld, size_t l_num, char* rego);
// tries to insert the key:value pair in the given level, returns true if successfull
// or false otherwise
bool insert_in_level(level_data_t *ld, size_t l_num, char* key, void* val);
// removes a rego from a given level incrementing the free_parks for that level 
res_t remove_from_level(level_data_t *ld, size_t l_num, char* key);
// returns true if the given key exists in the level referred to by l_num 
bool exists_in_level(level_data_t *ld, size_t l_num, char* key);
// returns the number of free parks in the level referred to by l_num
size_t levels_free_parks(level_data_t* ld, size_t l_num);

#endif
