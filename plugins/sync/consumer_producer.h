#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include <pthread.h>
#include "monitor.h"

/* Consumer–Producer queue structure */
typedef struct {
    char** items;            /* Array of string pointers */
    int capacity;            /* Maximum number of items */
    int count;               /* Current number of items */
    int head;                /* Index of first item */
    int tail;                /* Index of next insertion point */
    int alive;               /* 1 = alive, 0 = finished */

    pthread_mutex_t lock;    /* Shared lock for all operations */

    monitor_t not_full_monitor;   /* Monitor for "not full" state */
    monitor_t not_empty_monitor;  /* Monitor for "not empty" state */
    monitor_t finished_monitor;   /* Monitor for finished signal */
} consumer_producer_t;

int   consumer_producer_init(consumer_producer_t* q, int capacity);
void  consumer_producer_destroy(consumer_producer_t* q);

int   consumer_producer_put(consumer_producer_t* q, const char* item);
char* consumer_producer_get(consumer_producer_t* q);

void  consumer_producer_signal_finished(consumer_producer_t* q);
int   consumer_producer_wait_finished(consumer_producer_t* q);

#endif /* CONSUMER_PRODUCER_H */