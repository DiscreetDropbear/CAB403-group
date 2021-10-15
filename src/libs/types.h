#ifndef TYPES_H
#define TYPES_H

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

struct boomgate {
	pthread_mutex_t m;
	pthread_cond_t c;
	char state;
};

struct sign{
	pthread_mutex_t m;
	pthread_cond_t c;
	char display;
}

#endif
