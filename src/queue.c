#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct node{
	char * rego;
	struct node * next;
	struct node * prev;
} Node;

typedef struct queue{
	struct node * head;
	struct node * tail;
} Queue;

Queue * init_queue(){
	Queue * queue = (Queue*)malloc(sizeof(Queue));

	queue->head = NULL;
	queue->tail = NULL;

	return queue;
}

/* pushes a new node to the back of the queue
 * char * rego is assumed to be on the heap, no copying is done here
 */
void push(Queue *queue, char * rego){
	Node * node = (Node*)malloc(sizeof(Node));	
	if(node == NULL){
	// abort here
	}
	
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

int main() {
	
	// TESTS
	// 1. removing a node from an empty queue
	// 2. adding a new node to an empty queue
	// 3. removing a node form a queue with one node
	// 4. adding a new node to a queue with one node
	// 5. removing a node from a queue with two nodes

	Queue * queue = init_queue();
	
	// 1. removing a node from an empty queue
	char * res = pop(queue);
	assert(res == NULL);

	// 2. adding a new node to an empty queue
	char * r1 = "r1";
	push(queue, r1); 
	assert(queue->head == queue->tail);
	assert(queue->head->rego == r1);
	assert(queue->head->next == NULL);
	assert(queue->head->prev == NULL);

	// 3. removing a node form a queue with one node
	res = pop(queue);
	assert(res == r1);
	assert(queue->head == NULL);
	assert(queue->tail == NULL);
	
	// 4. adding a new node to a queue with one node
	char * r2 = "r2";
	push(queue, r1); 
	push(queue, r2);
	assert(queue->head != queue->tail);
	assert(queue->head->rego == r1);
	assert(queue->tail->rego == r2);
	assert(queue->head->next == queue->tail);
	assert(queue->tail->prev == queue->head);
	
	// 5. removing a node from a queue with two nodes
	res = pop(queue);
	assert(res == r1);
	assert(queue->head == queue->tail);
	assert(queue->head->rego == r2);
	assert(queue->head->next == NULL);
	assert(queue->head->prev == NULL);

	return 0;
}
