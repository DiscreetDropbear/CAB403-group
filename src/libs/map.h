#ifndef MAP_H
#define MAP_H

// the map assumes that all values stored within it are stored on the heap
// you shouldn't point to a value that lives on the stack or any global memory
// as it will try to free any values that are still left inside it when you 
// call free_map()

// the initial size of the map, will grow as needed, we won't implement shrinking as we won't need too
#define INIT_SIZE 8

// for now we will implement this map in a really niave way and if we need to optimise
// it for speed reasons we will

typedef struct map{
    int size;
    char** keys;         
    void** values;
} Map;

// sets up the map ready for use
void init_map(Map* map, unsigned int initial_size);

// free's all of the memory that map holds, including the values
void free_map(Map* map);

// inserts a key:value pair into the map returning the previous value
// if it exists
void* insert(Map* map, char* key, void* value); 

// removes a key:value pair from the map returning the value given the
// key exists in the map, other wise NULL is returned
void* remove(Map* map, char* key);

// returns true if the map has the given key
bool exists(Map* map, char* key);

#endif
