#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "map.h"

int find_key(Map* map, char* key);
int available_spot(Map* map);
void grow_map(Map* map);

// sets up the map ready for use
void init_map(Map* map, unsigned int initial_size){
    if( initial_size != 0){
        map->size = initial_size;
    }
    else{
        map->size = INIT_SIZE;
    }

    map->keys = calloc(map->size, sizeof(char**));
    map->values = calloc(map->size, sizeof(void**));
}

// free's all of the memory that the map holds, including the values.
void free_map(Map* map){
    for(int i = 0; i<(map->size); i++){
        if(map->keys[i] != NULL){
            free(map->keys[i]);
            // allow for values to be null
            if(map->values[i] != NULL){
                free(map->values[i]);
            }
        }
    }

    map->size = 0;
    free(map->keys);
    map->keys = NULL;
    free(map->values);
    map->values = NULL;
}

// inserts a key:value pair into the map returning the previous value
// if it exists
void* insert(Map* map, char* key, void* value){
    if(map == NULL || key == NULL || value == NULL){
        return NULL;
    }
    int index = find_key(map, key);
    int exists = 1;
    void* retVal;

    // key isn't in the map yet
    if(index == -1){
        index = available_spot(map);
        exists = 0;
       
        // no available spots
        if(index == -1){
            grow_map(map);
            index = available_spot(map);
            assert(index != -1);
        }
    }


    map->keys[index] = key;
    // save the value so we can return it
    retVal = map->values[index];
    map->values[index] = value;
    
    if(exists == 1){
        return retVal;
    }
    else{
        return NULL;
    }
}

// removes a key:value pair from the map returning the value given the
// key exists in the map, other wise NULL is returned
void* delete(Map* map, char* key){
    if(map == NULL || key == NULL){
        return NULL;
    }

    int index = find_key(map, key);
    int exists = 1;
    void* retVal;

    // key isn't in the map yet
    if(index == -1){
        return NULL;
    }

    map->keys[index] = NULL;
    // save the value so we can return it
    retVal = map->values[index];
    map->values[index] = NULL;
   
    return retVal;
}

// returns 1 if the map has the given key
// otherwise 0
int exists(Map* map, char* key){
    if(find_key(map, key) == -1){
        return 0;
    }

    return 1;
}

/*
 * The following functions are required for the implementation but
 * are not exposed as a part of the api
 */

// reallocates the memory for the map, it will double its size each time it grows 
void grow_map(Map* map){
    int new_size = map->size*2; 

    void* ret = realloc((void*)(map->keys), new_size*sizeof(char**));
    if(ret == NULL){
        exit(-1);
    }
    map->keys = ret;

    ret = realloc((void*)(map->values), new_size*sizeof(void**));
    if(ret == NULL){
        exit(-1);
    }
    
    map->values = ret;

    // make sure all of the new memory is initialised to 0
    // this only matters for the keys since the keys can be read when uninitialised
    // but values will only ever be read when initialised correctly
    for(int i = map->size; i < new_size; i++){
        map->keys[i] = NULL;  
    }

    map->size = new_size;
}

// returns the index of the first available spot or -1 if none exist
int available_spot(Map* map){
    // starting at the first element find the first element that is NULL
    // otherwise  
    for(int i = 0;i < map->size; i++){
        
        if(map->keys[i] == NULL){
            return i;
        }
    }

    return -1;
}

// returns the index where the key exists in the map or -1 if it doesn't exist in the map
int find_key(Map* map, char* key){
     for(int i = 0; i<map->size; i++){
        // make sure the strings in the map aren't null
        if(map->keys[i] != NULL){
            if(strcmp(map->keys[i], key) == 0){
                return i;
            }
        }
    }

    return -1;
}


