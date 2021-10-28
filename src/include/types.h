// types.h defines types and macros that will be used by all of the code, 
// any program (simulator, manager, firealarm) specific types needed will 
// be defined within their respective files to avoid sharing things that 
// arne't needed

#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>

#define ENTRANCES 5 
#define EXITS 5 
#define LEVELS 5 
#define LEVEL_CAPACITY 20

struct lpr {
    pthread_mutex_t m;
    pthread_cond_t c;
    char rego[6];
};

struct boom {
	pthread_mutex_t m;
	pthread_cond_t c;
	char state;
};

struct sign{
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
};

#endif
