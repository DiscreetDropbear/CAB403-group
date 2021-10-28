#ifndef BILLING_H
#define BILLING_H

#include <time.h>
#include <pthread.h>
#include "map.h"

typedef struct billing{
    Map existed;
    Map map;
} billing_t;

void init_billing(billing_t* billing);
void insert_rego(billing_t* billing, char* rego);
int remove_rego(billing_t* billing, char* rego, long unsigned* timespent);

#endif
