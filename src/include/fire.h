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


struct tempnode {
	int temperature;
	struct tempnode *next;
};

struct tempnode *deletenodes(struct tempnode *templist, int after)
{
	if (templist->next) {
		templist->next = deletenodes(templist->next, after - 1);
	}
	if (after <= 0) {
		free(templist);
		return NULL;
	}
	return templist;
}



void cb_init(ring_buffer *cb, size_t capacity, size_t size);

void cb_free(ring_buffer *cb);

void cb_push_back(ring_buffer *cb, const void *item);

void cb_pop_front(ring_buffer *cb, void *item);
