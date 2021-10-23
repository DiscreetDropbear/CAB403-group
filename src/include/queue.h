#ifndef QUEUE_H
#define QUEUE_H

typedef struct node{
	char * rego;
	struct node * next;
	struct node * prev;
} Node;

typedef struct queue{
	struct node * head;
	struct node * tail;
} Queue;

// sets up the queue ready for use
void init_queue(Queue* queue);

// frees any nodes on the queue including the values
void free_queue_nodes(Queue * queue);

// pushes rego onto the queue, rego is assumed to be stored on the heap, no copying is done
void push(Queue *queue, char * rego);

// pops the next value off of the queue
char * pop(Queue * queue);

#endif
