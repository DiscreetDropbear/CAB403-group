#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "../include/ring_buffer.h"


// initialises a ring buffer ready for use
// capacity is the maximum number of values the buffer 
// can hold
// size is the size of each of the values
// count is the number of items inside the ring buffer
int rb_init(ring_buffer_t* rb, size_t capacity){
    rb->buffer = calloc(capacity, sizeof(int));

    if(rb->buffer == NULL){
        fprintf(stderr, "unable to allocate memory for ring_buffer: errno(%d)\n", errno);
        return -1;
    }
    
    rb->buffer_end = rb->buffer + capacity * sizeof(int);
    rb->capacity = capacity;
    rb->count = 0;
    rb->head = rb->buffer;

    return 0;
}

// free's all the memory of the buffer and resets the
// other values to signify its not useable
void rb_free(ring_buffer_t* rb){
    free(rb->buffer);   // clear out other fields too, just to be safe
    rb->capacity = 0;
    rb->count = 0;
    rb->head = NULL;
}

// inserts an item into the buffer dealing with any
// wrapping
void rb_insert(ring_buffer_t* rb, int value){

    // insert the new value where head is pointing
    rb->head[0] = value;

    // increment head one place
    rb->head++;

    // if head has gone past the buffer
    // set it to point to the start
    if(rb->head > rb->buffer_end){
        rb->head = rb->buffer;
    }
    
    if(rb->count < rb->capacity){
       rb->count++;
    }
}

// returns the number of items currently
// in the buffer
int rb_count(ring_buffer_t* rb){
    return rb->count;
}

// returns the value at the given index or -1 if it doesn't exist
int rb_get_value(ring_buffer_t* rb, size_t index, int* ret){
    if(index > rb->capacity || index > rb->count){
        // the index given doesn't exists in the ring buffer 
        // or its currently unset
        return -1;
    }
    
    // get the index that the head is pointing to for the actual underlying buffer
    ptrdiff_t current_index = rb->head - rb->buffer; 
    
    // find the index of the value in the ring buffer
    size_t buffer_index = current_index - (index+1);

    if(buffer_index < 0){
        // index needs to wrap around and start at the end of the 
        // buffer
        buffer_index = rb->capacity - buffer_index;
    }

    // set the return value and return 0 to signify success
    *ret = rb->buffer[buffer_index];
    return 0;
}
























