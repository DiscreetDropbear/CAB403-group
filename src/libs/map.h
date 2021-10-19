#ifndef MAP_H
#define MAP_H
#include<stdbool.h>

// the map assumes that all values stored within it are stored on the heap
// you shouldn't point to a value that lives on the stack or any global memory
// as it will try to free any values that are still left inside it when you 
// call free_map()

// the initial size of the map, will grow as needed, we won't implement shrinking as we won't need too
#define INIT_SIZE 8 
// a percentage between 0 and 1 exclusive, specifies how many items to available spots there should
// be before growing the map to keep any buckets from having too long of a linked list
#define GROW_DENSITY 0.75

typedef struct pair pair_t;
struct pair{
    char* key;
    void* value;
};

typedef struct item item_t;
struct item{
    char* key;
    void* value;
    item_t *next;
};

typedef struct map Map; 
struct map{
    size_t size; // length of the buckets array
    size_t items; // number of key:value pairs currently in the map
    item_t ** buckets;
};

// returns the nth item that is in the map if 
// from start to finish
// this can be used to get a random value from the map
pair_t get_nth_item(Map* map, size_t n);

// sets up the map ready for use
void init_map(Map* map, unsigned int initial_size);

// free's all of the memory that map holds, including the values
void free_map(Map* map);

// inserts a key:value pair into the map returning the previous value
// if it exists
void* insert(Map* map, char* key, void* value); 

// removes a key:value pair from the map returning the value given the
// key exists in the map, other wise NULL is returned
void* remove_key(Map* map, char* key);

// returns true if the map has the given key
bool exists(Map* map, char* key);
#endif
