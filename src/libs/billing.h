#ifndef BILLING_H
#define BILLING_H

#include <time.h>
#include <pthread.h>
#include "map.h"

typedef struct billing{
    Map map;
} Billing;

void init_billing(Billing* billing);
void insert_rego(Billing* billing, char* rego);
timespec * remove_rego(Billing* billing, char* rego);

#endif
