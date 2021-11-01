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

size_t get_count(Map* map){
    assert(map != NULL); 
    return map->items;
}

pair_t get_random_item(Map* map){
    pair_t res;
    res.key = NULL;
    res.value = NULL;
    int seen = 0;

    // map is empty return NULL in res_t fields
    if(map->items == 0){
        return res;
    }

    // get the number of items we will see untill we return
    // one
    int item_n = rand() % map->items + 1;

    // visit each bucket in the map untill we see 'item_n' items 
    for(int i=0; i<map->size; i++){
        // within each bucket there can be a linked list of items  
        // we need to search through this linked list
        item_t* current = map->buckets[i];
        item_t* last = NULL;  
        // loop over this 
        while(current != NULL){
            seen++;
            if(seen == item_n){
                // we have found the item to return
                // remove it from the bucket and return it
                if(last == NULL){
                    // this is the first item in the bucket
                    // set the next item to the first
                    map->buckets[i] = current->next;
                }
                else{
                    // we are in the linked list somewhere
                    // join the last item, with the next item
                    last->next = current->next;  
                }

                // we have removed an item
                map->items--;
                // here current is removed from the linked list in the 
                // bucket so we can return its values
                res.key = current->key;
                res.value = current->value;
                
                return res;
            }

            // we haven't yet found the item
            // keep walking the linked list by going
            // to the next item and saving the last item
            // NOTE: if this is the last item in the list current will be
            // set to NULL after the next line and the while loop will exit
            // going to the next bucket
            last = current; 
            current = current->next;
        }
    }

    // TODO: remove this
    fprintf(stderr, "we shouldn't be here\n");
    exit(-1);
}

// returns a res_t holding the value if there is one for the given key.
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

    // check if the map needs to grow
    if(((double)map->items/(double)map->size) >= GROW_DENSITY){
        grow_map(map); 
    }
    
    // item to insert
    item_t * item = calloc(1, sizeof(item_t));
    // copy the key so the callers copy is still valid after the call
    int len = strlen(key)+1;
    item->key = calloc(1, len);
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

    if(map->items == 0){
        fprintf(stderr, "items is zero\n");
        exit(-1);
    }

    size_t index = get_index(map, key);
    res_t res;
    res.exists = false;
    res.value = NULL;
    
    item_t * current = map->buckets[index];
    item_t * last = NULL;

    // there is a linked list of items in this bucket
    // that we will need to search through 
    while(current != NULL){
        // found the key 
        if(strncmp(current->key, key, 6) == 0){
            // remove the current item from the list by joining
            // the previous item to the next one
            // Note: this is still valid even if current->next is NULL
            if(last != NULL){
                last->next = current->next; 
            }
            else{
                map->buckets[index] = current->next;
            }

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
    assert(key != NULL);

    size_t index = get_index(map, key);

    item_t * current = map->buckets[index];
    // there is a linked list of items in this bucket
    // that we will need to search through 
    while(current  != NULL){
        // found the key 
        if(strncmp(current->key, key, 6) == 0){
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
    assert(key != NULL);
    assert(map->size != 0);
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
