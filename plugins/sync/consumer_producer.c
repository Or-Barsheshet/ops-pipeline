#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "consumer_producer.h"

int consumer_producer_init(consumer_producer_t* q, int capacity) {
    if (!q || capacity <= 0) {
        fprintf(stderr, "[ERROR][queue] invalid queue or capacity\n");
        return -1;
    }

    q->capacity = capacity;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->alive = 0;

    q->items = (char**)malloc(sizeof(char*) * capacity);
    if (!q->items) {
        fprintf(stderr, "[ERROR][queue] items alloc failed\n");
        return -1;
    }

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        free(q->items);
        fprintf(stderr, "[ERROR][queue] mutex init failed\n");
        return -1;
    }

    // init monitors; unwind carefully on failure
    if (monitor_init(&q->not_full_monitor) != 0) {
        pthread_mutex_destroy(&q->lock);
        free(q->items);
        fprintf(stderr, "[ERROR][queue] not_full monitor init failed\n");
        return -1;
    }
    if (monitor_init(&q->not_empty_monitor) != 0) {
        monitor_destroy(&q->not_full_monitor);
        pthread_mutex_destroy(&q->lock);
        free(q->items);
        fprintf(stderr, "[ERROR][queue] not_empty monitor init failed\n");
        return -1;
    }
    if (monitor_init(&q->finished_monitor) != 0) {
        monitor_destroy(&q->not_empty_monitor);
        monitor_destroy(&q->not_full_monitor);
        pthread_mutex_destroy(&q->lock);
        free(q->items);
        fprintf(stderr, "[ERROR][queue] finished monitor init failed\n");
        return -1;
    }

    q->alive = 1;
    return 0;
}

void consumer_producer_destroy(consumer_producer_t* q) {
    if (!q) return;

    // mark dead and wake any waiters
    pthread_mutex_lock(&q->lock);
    q->alive = 0;
    monitor_signal_locked(&q->not_empty_monitor, &q->lock);
    monitor_signal_locked(&q->not_full_monitor, &q->lock);
    monitor_signal_locked(&q->finished_monitor, &q->lock);
    pthread_mutex_unlock(&q->lock);

    // destroy monitors and lock
    monitor_destroy(&q->finished_monitor);
    monitor_destroy(&q->not_empty_monitor);
    monitor_destroy(&q->not_full_monitor);
    pthread_mutex_destroy(&q->lock);

    // free buffer
    free(q->items);
    q->items = NULL;
    q->capacity = q->count = q->head = q->tail = 0;
}

int consumer_producer_put(consumer_producer_t* q, const char* item) {
    if (!q || !item) {
        fprintf(stderr, "[ERROR][queue] put: invalid args\n");
        return -1;
    }

    pthread_mutex_lock(&q->lock);
    // block while full and alive
    while (q->alive && (q->count == q->capacity)) {
        if (monitor_wait_locked(&q->not_full_monitor, &q->lock) != 0) {
            pthread_mutex_unlock(&q->lock);
            return -1;
        }
    }
    if (!q->alive) { pthread_mutex_unlock(&q->lock); return -1; }

    // copy item into ring buffer
    char* copy = strdup(item);
    if (!copy) { pthread_mutex_unlock(&q->lock); return -1; }

    q->items[q->tail] = copy;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    // notify a potential getter
    monitor_signal_locked(&q->not_empty_monitor, &q->lock);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

char* consumer_producer_get(consumer_producer_t* q) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->lock);
    // block while empty and alive
    while (q->alive && (q->count == 0)) {
        if (monitor_wait_locked(&q->not_empty_monitor, &q->lock) != 0) {
            pthread_mutex_unlock(&q->lock);
            return NULL;
        }
    }
    // if dead and empty, nothing to return
    if (!q->alive && q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    // take one item from ring buffer
    char* item = q->items[q->head];
    q->items[q->head] = NULL;
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    // notify a potential putter
    monitor_signal_locked(&q->not_full_monitor, &q->lock);
    pthread_mutex_unlock(&q->lock);
    return item;
}

void consumer_producer_signal_finished(consumer_producer_t* q) {
    if (!q) return;

    pthread_mutex_lock(&q->lock);
    q->alive = 0;
    // wake everyone waiting on any condition
    monitor_signal_locked(&q->not_empty_monitor, &q->lock);
    monitor_signal_locked(&q->not_full_monitor,  &q->lock);
    monitor_signal_locked(&q->finished_monitor,  &q->lock);
    pthread_mutex_unlock(&q->lock);
}

int consumer_producer_wait_finished(consumer_producer_t* q) {
    if (!q) return -1;
    pthread_mutex_lock(&q->lock);
    int rc = monitor_wait_locked(&q->finished_monitor, &q->lock);
    pthread_mutex_unlock(&q->lock);
    return rc;
}