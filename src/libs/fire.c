#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "../include/fire.h"


void cb_init(ring_buffer *cb, size_t capacity, size_t size){
	
    cb->buffer = malloc(capacity * size);

    if(cb->buffer == NULL){
        //handling error
    }
    
    cb->buffer_end = (char *)cb->buffer + capacity * size;
    cb->capacity = capacity;
    cb->count = 0;
    cb->size = size;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
    
}

void cb_free(ring_buffer *cb){

    free(cb->buffer);   // clear out other fields too, just to be safe
}

void cb_push_back(ring_buffer *cb, const void *item){

    memcpy(cb->head, item, cb->size);
    cb->head = (char*)cb->head + cb->size;
    if(cb->head == cb->buffer_end){
        cb->head = cb->buffer;
    }
    
    if (cb->count < cb->capacity){
       cb->count++;
    }
}

void cb_pop_front(ring_buffer *cb, void *item){

    memcpy(item, cb->tail, cb->size);
    cb->tail = (char*)cb->tail + cb->size;
    if(cb->tail == cb->buffer_end){
        cb->tail = cb->buffer;
    }
    
    cb->count--;
}
