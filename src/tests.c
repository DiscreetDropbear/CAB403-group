/* runs tests for libraries and other code 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libs/map.h"
#include "assert.h"

void run_map_tests();

int main(){
    
    // map tests
    printf("running map tests\n");
    run_map_tests();
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
    memcpy(keys[0], "aaaa", 5); 
    keys[0][5] = 0;
    memcpy(keys[1], "bbbb", 5); 
    keys[1][5] = 0;
    memcpy(keys[2], "cccc", 5); 
    keys[2][5] = 0;
    memcpy(keys[3], "dddd", 5); 
    keys[3][5] = 0;
    memcpy(keys[4], "eeee", 5); 
    keys[4][5] = 0;
    
    int * values = malloc(sizeof(int)*5);
    values[0] = 0;
    values[1] = 1;
    values[2] = 2;
    values[3] = 3;
    values[4] = 4;

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
    assert(map.size == 2);
    insert(&map, keys[2], &values[2]);  
    assert(map.size == 4);
    insert(&map, keys[3], &values[3]);  
    insert(&map, keys[4], &values[4]);  
    assert(map.size == 8);


    // map retrieves a key thats in the map
    // map returns null when searching for a key that isn't in the map
    
    // returns the old value when inserting a key that already exists

    // find_key returns the correct index when 
    //
}
