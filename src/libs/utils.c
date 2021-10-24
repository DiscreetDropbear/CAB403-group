#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "../include/utils.h"

// TODO: take into account the sleep modifier here so in testing
// this will still work
int time_diff(struct timespec before, unsigned long * milli){
    assert(milli != NULL);
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    
    // failure
    if(res != 0){
        return res;
    }

    //success
    assert(before.tv_sec <= now.tv_sec);
    unsigned long diff_milli = (now.tv_sec - before.tv_sec) * 1000; 

    if(now.tv_nsec > before.tv_nsec){
        diff_milli += (unsigned long)((now.tv_nsec - before.tv_nsec)/1000000);    
    }
    else{
        diff_milli += (unsigned long)((before.tv_nsec - now.tv_nsec)/1000000);    
    }
    
    *milli = diff_milli;

    return 0;
}

int get_regos( char** * regos){
    //have to find number of lines in file
    FILE *fp;
    size_t n = 0; 
    char c;  // To store a character read from file
  
    // Open the file
    fp = fopen("plates.txt", "r");
  
    // Check if file exists
    if (fp == NULL){
        printf("Could not open file");
        return 0;
    }
  
    // Extract characters from file and store in character c
    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n') {
            n = n + 1;
        }
    }

    fclose(fp);
  
    int a = 0;
    char ** values = malloc(sizeof(char*) * n); 
    assert(values != NULL);

    for(int i = 0; i<n; i++){
        values[i] = malloc(sizeof(char) * 7);
        assert(values[i] != NULL); 
    }

    FILE *archivo = fopen("plates.txt","r");    

    if (archivo == NULL){
        exit(1);
    }

    for (feof(archivo) == 0){
        fscanf( archivo, "%c%c%c%c%c%c\n", &values[a][0],&values[a][1],&values[a][2],&values[a][3],&values[a][4],&values[a][5]);
        a++;
    }

    fclose(archivo);
    return 0;
}
