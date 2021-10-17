#include "map.h"

void map_tests(){
    Map map;

    init_map(&map, 0);
   
    // map init works correctly
  
    // free_map free's all of the memory successfully including the values
    // that are still stored in the map

    // map grows as needed 

    // map retrieves a key thats in the map
    // map returns null when searching for a key that isn't in the map
    
    // returns the old value when inserting a key that already exists

    // find_key returns the correct index when 
    //
}


// reallocates the memory for the map, it will double its size each time it grows 
void grow_map(Map* map){
    map->size = map->size; 
    void* ret = realloc((void*)map->keys, map->size*2);
    if(ret == NULL){
        printf("reallocation for a map failed!");
        exit(-1);
    }
    map->keys = (char**)

    void* ret = realloc((void*)map->values, map->size*2);
    if(ret == NULL){
        printf("reallocation for a map failed!");
        exit(-1);
    }

    // make sure all of the new memory is initialised to 0
    // this only matters for the keys since the keys can be read when uninitialised
    // but values will only ever be read when initialised correctly
    for(int i = map->size; i < map->size*2; i++){
        map->keys+i = NULL;  
    }
}

// returns the index of the first available spot or -1 if none exist
int available_spot(Map* map){
    // starting at the first element find the first element that is NULL
    // otherwise  
    for(int i = 0;i < map->size; i++){
        
        if((map->keys)+i == NULL){
            return i;
        }
    }

    return -1;
}

// returns the index where the key exists in the map or -1 if it doesn't exist in the map
int find_key(Map* map, char* key){
     for(int i = 0; i<(map->size); i++){
         // make sure the strings in the map aren't null
        if((map->keys)+i != NULL){
            if(strcmp((map->keys)+i, key) == 0){
                return i;
            }
        }
    }

    return -1;
}

// sets up the map ready for use
void init_map(Map* map, unsigned int initial_size){
    if( initial_size != 0){
        map->size = initial_size;
    }
    else{
        map->size = INIT_SIZE;
    }
    map->keys = (char*)malloc(sizeof(char**)*(map->size));
    map->values = malloc(sizeof(void**)*(map->size));
}

// free's all of the memory that the map holds, including the values.
void free_map(Map* map){
    for(int i = 0; i<(map->size); i++){
        if((map->keys)+i != NULL){
            free((map->keys)+i);
            // allow for values to be null
            if((map->values)+i != NULL){
                free((map->values)+i);
            }
        }
    }

    free(map->keys);
    free(map->values);
}

// inserts a key:value pair into the map returning the previous value
// if it exists
void* insert(Map* map, char* key, void* value){
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
        }
    }

    (map->keys)+index = key;
    // save the value so we can return it
    retVal = (map->values)+index;
    (map->values)+index = value;
    
    if(exists == 1){
        return retVal;
    }
    else{
        return NULL;
    }
}



// removes a key:value pair from the map returning the value given the
// key exists in the map, other wise NULL is returned
void* remove(Map* map, char* key){
    int index = find_key(map, key);
    int exists = 1;
    void* retVal;

    // key isn't in the map yet
    if(index == -1){
        return NULL;
    }

    (map->keys)+index = NULL;
    // save the value so we can return it
    retVal = (map->values)+index;
    (map->values)+index = NULL;
   
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
