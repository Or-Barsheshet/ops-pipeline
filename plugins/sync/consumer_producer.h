#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include "monitor.h"

typedef struct {
    char** items;
    int capacity;
    int count;
    int head;
    int tail;
    int alive;   // 1=alive, 0=finished

    monitor_t not_full_monitor;
    monitor_t not_empty_monitor;
    monitor_t finished_monitor;
} consumer_producer_t;

int   consumer_producer_init(consumer_producer_t* queue, int capacity);
void  consumer_producer_destroy(consumer_producer_t* queue);
int   consumer_producer_put(consumer_producer_t* queue, const char* item);
char* consumer_producer_get(consumer_producer_t* queue);
void  consumer_producer_signal_finished(consumer_producer_t* queue);
int   consumer_producer_wait_finished(consumer_producer_t* queue);

#endif