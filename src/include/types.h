// types.h defines types and macros that will be used by all of the code, 
// any program (simulator, manager, firealarm) specific types needed will 
// be defined within their respective files to avoid sharing things that 
// arne't needed

#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>

#define ENTRANCES 1 
#define EXITS 1 
#define LEVELS 1 
#define LEVEL_CAPACITY 20

struct lpr_t {
    pthread_mutex_t m;
    pthread_cond_t c;
    char rego[6];
};

struct boom_t {
	pthread_mutex_t m;
	pthread_cond_t c;
	char state;
};

struct sign_t{
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
};

#endif
