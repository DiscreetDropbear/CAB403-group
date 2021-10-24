#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "../include/map.h"

int find_key(Map* map, char* key);
int available_spot(Map* map);
void grow_map(Map* map);
void free_item_list(item_t * item);
res_t internal_insert(Map* map, item_t * item);
item_t* internal_search(Map*map, char* key);
size_t get_index(Map* map, char* key);

// sets up the map ready for use
void init_map(Map* map, unsigned int initial_size){
    assert(map != NULL);

    if( initial_size != 0){
        map->size = initial_size;
    }
    else{
        map->size = INIT_SIZE;
    }
    
    map->items = 0;
    map->buckets = calloc(map->size, sizeof(item_t*));
}

// free's all of the memory that the map holds, including the values.
void free_map(Map* map){
    // free's any remaining items in the buckets
    for(size_t i = 0; i < map->size; i++){
        if(map->buckets[i] != NULL){
            free_item_list(map->buckets[i]);
        }
    }

    // free the buckets array 
    free(map->buckets);
}

pair_t get_nth_item(Map* map, size_t n){
    pair_t pair = {key: NULL, value: NULL};
    if(map->items == 0){
        return pair;         
    }

    // since there may not be n items in the map
    // wrap the number around how many items there are
    // to get a number in the range 0 to (map->items-1)
    size_t item_n = n % map->items;

    // keeps finding the next item untill
    // found_count == item_n
    size_t found_count = 0;
    item_t * current;
    item_t * last = NULL;

    for(int i = 0; i < map->size; i++){

        current = map->buckets[i]; 
        // there is a linked list of items in this bucket
        // that we will need to search through 
        while(current != NULL){
                found_count++;
                if(found_count == n){
                    // remove the current item from the list by joining
                    // the previous item to the next one
                    if(last->next == NULL){ // means this is the first item in the bucket
                        map->buckets[i] = current->next; 
                    } 
                    else{
                        last->next = current->next; 
                    }

                    // put a pair_t on the heap ready for returning
                    pair.key = current->key;
                    pair.value = current->value;

                    // free the item_t
                    current->next = NULL;
                    free(current);

                    map->items--;
                    return pair;
                }
            
            last = current;
            current = current->next;
        }    
    }

    return pair;
}

// returns a res_t holding the value if there is one for the given key.
//
// NOTE: this value is not copied and referres to the same memory as the one in the
// map, this means if some other thread removes this rego and free's the value that
// this will be an invalid pointer, the result should only be dereferenced when you 
// hold an exclusive lock to the map and once you release this lock all bets are off
// as to the the validity of the pointer
//
// THE VALUE POINTER RETURNED IN RES_T MUST NOT BE FREED AS ITS STILL USED BY THE MAP
// AND SHOULD BE FREED BY THE CALLER TO remove_key
res_t search(Map* map, char* key){
    assert(map->size != 0);
    assert(key != NULL);
    item_t* item =  internal_search(map, key);

    res_t res;
    if(item != NULL){
        res.exists = true;
        res.value = item->value;
        return res;
    }

    res.exists = false;
    res.value = NULL;
    return res;
}

// inserts a key:value pair into the map returning the previous value
// if it exists
res_t insert(Map* map, char* key, void* value){
    assert(map->size != 0);
    assert(key != NULL);
    assert(value != NULL);

    // check if the map needs to grow
    if(((double)map->items/(double)map->size) >= GROW_DENSITY){
        grow_map(map); 
    }
    
    // item to insert
    item_t * item = calloc(1, sizeof(item_t));
    // copy the key so the callers copy is still valid after the call
    int len = strlen(key)+1;
    item->key = malloc(len);
    memcpy(item->key, key, len); 

    item->value = value;
    item->next = NULL;
    
    // internal_insert will return a pointer to the value if the
    // key already existed in the map
    return internal_insert(map, item);
}

// removes a key:value pair from the map returning either value given the
// key exists or NULL otherwise 
// Note: it is up to the caller to free any value that is returned
res_t remove_key(Map* map, char* key){
    assert(map != NULL);  
    assert(key != NULL);
        
    size_t index = get_index(map, key);
    res_t res;
    res.exists = false;
    res.value = NULL;
    
    // there are no items in the bucket
    // the key doesn't exist
    if(map->buckets[index] == NULL){
        return res;
    }
   
    // the key is the first item in the bucket
    if(strcmp(map->buckets[index]->key, key) == 0){
        item_t * tmp = map->buckets[index];
        map->buckets[index] = tmp->next;
        
        // save the value, free the key and then the item
        res.exists = true;
        res.value = tmp->value;

        free(tmp->key);
        free(tmp);

        map->items--;
        return res;
    }
    

    item_t * current = map->buckets[index]->next;
    item_t * last = map->buckets[index];

    // there is a linked list of items in this bucket
    // that we will need to search through 
    while(current != NULL){
        // found the key 
        if(strcmp(current->key, key) == 0){
            // remove the current item from the list by joining
            // the previous item to the next one
            // Note: this is still valid even if current->next is NULL
            last->next = current->next; 


            res.exists = true;
            res.value = current->value;

            free(current->key);
            free(current);

            map->items--;
            return res;
        }

        
        last = current;
        current = current->next;
    }

    // nothing was found
    return res;
}

bool exists(Map* map, char* key){
    assert(map != NULL);
    size_t index = get_index(map, key);

    // there are no items in the bucket
    // the key doesn't exist
    if(map->buckets[index] == NULL){
        return false;
    }
   
    // the key is the first item in the bucket
    if(strcmp(map->buckets[index]->key, key) == 0){
        return true;
    }
    
    item_t * current;
    // there is a linked list of items in this bucket
    // that we will need to search through 
    while((current = map->buckets[index]->next) != NULL){

        // found the key 
        if(strcmp(current->key, key) == 0){
            return true;
        }
        
        current = current->next;
    }

    return false;
}


//
/// THE FOLLOWING FUNCTIONS ARE NOT A PART OF THE PUBLIC API
//

// This function was taken from the hashtable.c file in the week 3 practical!
// The Bernstein hash function.
// A very fast hash function that works well in practice.
size_t djb_hash(char* s)
{
    size_t hash = 5381;
    int c;
    while ((c = *s++) != '\0')
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// This function was taken from the hashtable.c file from the week 3 practical!
// Calculate the offset for the bucket for key in hash table.
size_t get_index(Map* map, char* key)
{
    return djb_hash(key) % map->size;
}

// safely frees a list of items
// Note: this is still safe if the item is the only
// item in the list
void free_item_list(item_t * item){
    assert(item != NULL); 
    item_t * next;

    do{
        next = item->next;

        free(item->key);
        free(item->value);
        free(item);

        item = next;
    }while(next != NULL);
}


// this function implements all of the business logic behind inserts
// this is done so we don't need to re-implement this in the grow_map 
// function and we don't need to free items and make new ones by using
// the insert method from the api which only takes key and value
// pointers 
//
// increments map->items if this is a new item
res_t internal_insert(Map* map, item_t * item){
    res_t res;
    res.exists = false;
    res.value = NULL;

    // find the key if it already exists
    item_t* ret = internal_search(map, item->key);
    if(ret != NULL){
        // get the old value so we can return it
        res.value = ret->value;
        res.exists = true;
        // insert the new value into the item 
        ret->value = item->value;

        // free the item that was passed in as
        // we don't need it anymore
        // Note: item->value is still being used
        
        free(item->key);
        free(item);
        return res;
    }

    size_t index = get_index(map, item->key); 
    if(map->buckets[index] != NULL){
        // set items next to the item which is currently pointed 
        // to by map->buckets[index] so we can point to the item from
        // the bucket
        item->next = map->buckets[index];
    }

    // pointing to the item from the bucket, if there was a value before then
    // the item that was first is now pointed to by item->next which means that
    // item is now the "head" of the list
    map->buckets[index] = item;

    // this is a new item so add it to the count
    map->items++;
    // there was no previous value so there is nothing
    // to return
    return res;
}

// returns a pointer to the item_t that holds the key
item_t* internal_search(Map*map, char* key){
    assert(map != NULL);
    assert(key != NULL);
    size_t index = get_index(map, key);

    // there are no items in the bucket
    // so the key doesn't exist
    if(map->buckets[index] == NULL){
        return NULL;
    }
   
    // the key is the first item in the bucket
    if(strcmp(map->buckets[index]->key, key) == 0){
        return map->buckets[index];
    }
    
    item_t * current = map->buckets[index]->next;
    // there is a linked list of items in this bucket
    // that we will need to search through 
    while(current != NULL){
        // found the key 
        if(strcmp(current->key, key) == 0){
            return current;
        }
        
        current = current->next;
    }

    return NULL;
}

// reallocates the memory for the map, it will double its size each time it grows 
void grow_map(Map* map){
    int new_size = map->size*2; 
    int old_size = map->size;
    
    // allocate a new array
    item_t ** tmp = calloc(new_size, sizeof(item_t *));

    // switch the old with the new
    item_t ** old = map->buckets; 
    map->buckets = tmp;
    map->size = new_size;
    map->items = 0;// this will be incremented each time
                   // internal_insert() is called

    item_t * next;
    item_t * current;
    // loop over the buckets array and recalculate the index for each
    // item inserting them into the new array.
    for(int i = 0; i< old_size; i++){
        // reassign each item in the list to the new array
        current = old[i];
        while(current != NULL){
            // save the next item if there is one
            if((next = current->next) != NULL){
                // remove the link to the other item
                current->next = NULL;
            }
            
            internal_insert(map, current);  
           
            // current is now inserted into the new map so its safe
            // to let go of the pointer,
            // when next is NULL this will cancel the while loop
            current = next;
        }
    }
}
