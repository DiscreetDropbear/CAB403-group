#ifndef FIRE_H
#define FIRE_H

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

typedef struct ring_buffer{
	
    void *buffer;     
    void *buffer_end; 
    size_t capacity; 
    size_t count;     
    size_t size; 
    void *head;     
    void *tail;         
      
} ring_buffer;


void cb_init(ring_buffer *cb, size_t capacity, size_t size);
