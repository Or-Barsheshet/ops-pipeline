#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include <pthread.h>
#include "monitor.h"

// bounded queue for strings with external lock + monitors
typedef struct {
    char** items;                 // array of string pointers (heap)
    int capacity;                 // max number of items
    int count;                    // current number of items
    int head;                     // index of next item to take
    int tail;                     // index of next slot to fill
    int alive;                    // 1 = running, 0 = finished

    pthread_mutex_t lock;         // single lock for all ops

    monitor_t not_full_monitor;   // signal when space is available
    monitor_t not_empty_monitor;  // signal when item is available
    monitor_t finished_monitor;   // signal final shutdown
} consumer_producer_t;

int   consumer_producer_init(consumer_producer_t* q, int capacity);
void  consumer_producer_destroy(consumer_producer_t* q);

int   consumer_producer_put(consumer_producer_t* q, const char* item);
char* consumer_producer_get(consumer_producer_t* q);

void  consumer_producer_signal_finished(consumer_producer_t* q);
int   consumer_producer_wait_finished(consumer_producer_t* q);

#endif // CONSUMER_PRODUCER_H