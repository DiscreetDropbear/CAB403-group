#ifndef MAP_H
#define MAP_H
#include<stdbool.h>

// the initial size of the map, will grow as needed, we won't implement shrinking as we won't need too
#define INIT_SIZE 1024 
// a percentage between 0 and 1 exclusive, specifies how many items to available spots there should
// be before growing the map to keep any buckets from having too long of a linked list
#define GROW_DENSITY 0.75

typedef struct pair pair_t;
struct pair{
    char* key;
    void* value;
};

// result type for remove_from_level
// so we can tell if it was in the level or not
// and what the return value was if it was in the 
// level
typedef struct res{
    bool exists;
    void* value;
} res_t;

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


size_t get_count(Map *map);

// returns a res_t holding the value if there is one for the given key.
//
// NOTE: this value is not copied and referres to the same memory as the one in the
// map, this means if some other thread removes this rego and free's the value that
// this will be an invalid pointer, the result should only be dereferenced when you 
// hold an exclusive lock to the map and once you release this lock all bets are off
// as to the the validity of the pointer
// THE VALUE POINTER IN RES_T MUST NOT BE FREED AS ITS STILL USED BY THE MAP
res_t search(Map* map, char* key); 

// inserts a key:value pair into the map returning the previous value
// if it exists
res_t insert(Map* map, char* key, void* value); 

// removes a key:value pair from the map returning the value given the
// key exists in the map, other wise NULL is returned
res_t remove_key(Map* map, char* key);

// returns true if the map has the given key
bool exists(Map* map, char* key);
#endif
