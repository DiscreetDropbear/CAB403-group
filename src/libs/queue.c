#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "queue.h"

void init_queue(Queue * queue){
    assert(queue != NULL);
	queue->head = NULL;
	queue->tail = NULL;
}

void free_queue_nodes(Queue * queue){
    assert(queue != NULL);

    char * val;
    while((val = pop(queue)) != NULL){
        // free the value memory
        free(val);  
    }
}

/* pushes a new node to the back of the queue
 * char * rego is assumed to be on the heap, no copying is done here
 */
void push(Queue *queue, char * rego){
    assert(queue != NULL);
	Node * node = (Node*)malloc(sizeof(Node));	
	
	node->rego = rego;
	node->next = NULL;
	node->prev = NULL;

	if(queue->head == NULL){
		queue->head = node;
		queue->tail = node;
		return;
	}
	
	node->prev = queue->tail;

	queue->tail->next = node;
	queue->tail = node;
}

char * pop(Queue * queue){
    assert(queue != NULL);
	// there are no nodes in the queue
	if(queue->head == NULL){
		return NULL;
	}
	
	char * rego = queue->head->rego;

	// there is only one node in the queue
	if(queue->head->next == NULL){
		free(queue->tail);
		queue->tail = NULL;
		queue->head = NULL;
	}
	else{// there are more than one node in the queue	
		// the node after head will become the new head,
		// and so we need to set its prev to NULL
		queue->head->next->prev = NULL;	
		// save the current head so we can free the node 
		Node * previous_head = queue->head;
		// set the node after head to be head
		queue->head = queue->head->next;

		free(previous_head);
	}

	return rego;
}
