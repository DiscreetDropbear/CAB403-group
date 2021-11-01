#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

typedef struct ring_buffer{
    int *buffer;     
    int *buffer_end; 
    size_t capacity; 
    size_t count;     
    int *head;     
} ring_buffer_t;

int rb_init(ring_buffer_t*, size_t);

void rb_free(ring_buffer_t *);

void rb_insert(ring_buffer_t *, int);

int rb_count(ring_buffer_t* rb);

int rb_get_value(ring_buffer_t* rb, size_t index, int* ret);

#endif
