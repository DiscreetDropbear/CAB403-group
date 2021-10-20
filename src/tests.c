/* runs tests for libraries and other code 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libs/map.h"
#include "libs/queue.h"
#include "assert.h"

void run_map_tests();
void run_queue_tests();

int main(){
    
    // map tests
    printf("running map tests\n");
    run_map_tests();

    // queue tests
    printf("running queue tests\n");
    run_queue_tests();
}

void run_map_tests(){
    Map map;
    
    // create an array of string pointers
    char ** keys = (char**)malloc(sizeof(char*)*5);
    keys[0] = malloc(5);
    keys[1] = malloc(5);
    keys[2] = malloc(5);
    keys[3] = malloc(5);
    keys[4] = malloc(5);
    keys[5] = malloc(5);
    memcpy(keys[0], "aaaa\0", 5); 
    memcpy(keys[1], "bbbb\0", 5); 
    memcpy(keys[2], "cccc\0", 5); 
    memcpy(keys[3], "dddd\0", 5); 
    memcpy(keys[4], "eeee\0", 5); 
    memcpy(keys[5], "ffff\0", 5); 
    
    int * values = malloc(sizeof(int)*6);
    values[0] = 0;
    values[1] = 1;
    values[2] = 2;
    values[3] = 3;
    values[4] = 4;
    values[5] = 5;
    /// map init works correctly
    // map init with a supplied start size 
    
    init_map(&map, 2);
    assert(map.size == 2);
    free_map(&map);

    // map init without supplied start size
    init_map(&map, 0);
    assert(map.size == INIT_SIZE);
    free_map(&map);

    // free_map free's all of the memory successfully including the values
    // that are still stored in the map
    //TODO 

    // map grows as needed 
    init_map(&map, 1);

    insert(&map, keys[0], &values[0]);  
    insert(&map, keys[1], &values[1]);  
    insert(&map, keys[2], &values[2]);  
    insert(&map, keys[3], &values[3]);  
    insert(&map, keys[4], &values[4]);  
    insert(&map, keys[5], &values[5]);  
    assert(map.size > 1);

    // deleting a key thats in the map returns the old value 
    void * oldv = remove_key(&map, keys[0]);
    assert(oldv == &values[0]); 

    // map returns null when deleting a key that isn't in the map
    oldv = remove_key(&map, keys[0]);
    assert(oldv == NULL); 

    // returns the old value when inserting a key that already exists
    oldv = insert(&map, keys[1], &values[0]);  
    assert(oldv == &values[1]);

    pair_t pair = get_nth_item(&map, 5);
    assert(pair.key != NULL);
    assert(pair.value != NULL);
}

void run_queue_tests() {
	
	// TESTS
	// 1. removing a node from an empty queue
	// 2. adding a new node to an empty queue
	// 3. removing a node form a queue with one node
	// 4. adding a new node to a queue with one node
	// 5. removing a node from a queue with two nodes
    // 6. adding a sequence of nodes and poping them off making sure they are in the correct order
    // 7. test freeing queue nodes an empty queue
    // 8. test freeing queue nodes with one element
    // 9. test freeing queue nodes with multiple elements
	
    Queue queue;
    init_queue(&queue);
	
	// 1. removing a node from an empty queue
	char * res = pop(&queue);
	assert(res == NULL);

	// 2. adding a new node to an empty queue
	char * r1 = "r1";
	push(&queue, r1); 
	assert(queue.head == queue.tail);
	assert(queue.head->rego == r1);
	assert(queue.head->next == NULL);
	assert(queue.head->prev == NULL);

	// 3. removing a node form a queue with one node
	res = pop(&queue);
	assert(res == r1);
	assert(queue.head == NULL);
	assert(queue.tail == NULL);
	
	// 4. adding a new node to a queue with one node
	char * r2 = "r2";
	push(&queue, r1); 
	push(&queue, r2);
	assert(queue.head != queue.tail);
	assert(queue.head->rego == r1);
	assert(queue.tail->rego == r2);
	assert(queue.head->next == queue.tail);
	assert(queue.tail->prev == queue.head);
	
	// 5. removing a node from a queue with two nodes
	res = pop(&queue);
	assert(res == r1);
	assert(queue.head == queue.tail);
	assert(queue.head->rego == r2);
	assert(queue.head->next == NULL);
	assert(queue.head->prev == NULL);
    // TODO:6. adding a sequence of nodes and poping them off making sure they are in the correct order
    // TODO:7. test freeing queue nodes an empty queue
    // TODO:8. test freeing queue nodes with one element
    // TODO:9. test freeing queue nodes with multiple elements
}

