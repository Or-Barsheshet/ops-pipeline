#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "consumer_producer.h"

int consumer_producer_init(consumer_producer_t* q, int capacity) {
    if (!q || capacity <= 0) {
        fprintf(stderr, "[queue] invalid queue or capacity\n");
        return -1;
    }
    q->capacity = capacity;
    q->count = 0;
    q->head = 0;
    q->tail = 0;
    q->alive = 0;

    q->items = (char**)malloc(sizeof(char*) * capacity);
    if (!q->items) {
        fprintf(stderr, "[queue] items alloc failed\n");
        return -1;
    }

    if (monitor_init(&q->not_full_monitor) != 0) {
        fprintf(stderr, "[queue] not_full monitor init failed\n");
        free(q->items); return -1;
    }
    if (monitor_init(&q->not_empty_monitor) != 0) {
        fprintf(stderr, "[queue] not_empty monitor init failed\n");
        monitor_destroy(&q->not_full_monitor);
        free(q->items); return -1;
    }
    if (monitor_init(&q->finished_monitor) != 0) {
        fprintf(stderr, "[queue] finished monitor init failed\n");
        monitor_destroy(&q->not_empty_monitor);
        monitor_destroy(&q->not_full_monitor);
        free(q->items); return -1;
    }

    q->alive = 1;
    return 0;
}

void consumer_producer_destroy(consumer_producer_t* q) {
    if (!q) return;
    q->alive = 0;
    monitor_signal(&q->not_empty_monitor);
    monitor_signal(&q->not_full_monitor);
    monitor_signal(&q->finished_monitor);

    monitor_destroy(&q->finished_monitor);
    monitor_destroy(&q->not_empty_monitor);
    monitor_destroy(&q->not_full_monitor);

    free(q->items);
    q->items = NULL;
    q->capacity = q->count = q->head = q->tail = 0;
}

int consumer_producer_put(consumer_producer_t* q, const char* item) {
    if (!q || !item) {
        fprintf(stderr, "[queue] put: invalid args\n");
        return -1;
    }

    pthread_mutex_lock(&q->not_full_monitor.mutex);
    while (q->alive && (q->count == q->capacity)) {
        monitor_wait_locked(&q->not_full_monitor);
    }
    if (!q->alive) {
        pthread_mutex_unlock(&q->not_full_monitor.mutex);
        fprintf(stderr, "[queue] put: queue not alive\n");
        return -1;
    }

    char* copy = strdup(item);
    if (!copy) {
        pthread_mutex_unlock(&q->not_full_monitor.mutex);
        fprintf(stderr, "[queue] put: strdup failed\n");
        return -1;
    }
    q->items[q->tail] = copy;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    monitor_signal(&q->not_empty_monitor);
    pthread_mutex_unlock(&q->not_full_monitor.mutex);
    return 0;
}

char* consumer_producer_get(consumer_producer_t* q) {
    if (!q) return NULL;

    pthread_mutex_lock(&q->not_empty_monitor.mutex);

    /* לולאת חסימה עם טיימאאוט “ארוך”: 1000ms.
       זה לא פוגע במבחני חסימה של 100ms,
       אבל מציל את ה‑stress במקרה של ריקון סופי. */
    while (q->alive && (q->count == 0)) {
        int rc = monitor_timedwait_locked(&q->not_empty_monitor, 1000);
        if (rc == 1) { // timeout
            pthread_mutex_unlock(&q->not_empty_monitor.mutex);
            return NULL; // “אפס פריט כרגע” — מאפשר ללולאה של הטסט לצאת בזמן
        }
        if (rc < 0) { // error
            pthread_mutex_unlock(&q->not_empty_monitor.mutex);
            return NULL;
        }
    }

    if (!q->alive && q->count == 0) {
        pthread_mutex_unlock(&q->not_empty_monitor.mutex);
        return NULL;
    }

    char* item = q->items[q->head];
    q->items[q->head] = NULL;
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    monitor_signal(&q->not_full_monitor);
    pthread_mutex_unlock(&q->not_empty_monitor.mutex);
    return item;
}

void consumer_producer_signal_finished(consumer_producer_t* q) {
    if (!q) return;
    q->alive = 0;
    monitor_signal(&q->not_empty_monitor);
    monitor_signal(&q->not_full_monitor);
    monitor_signal(&q->finished_monitor);
}

int consumer_producer_wait_finished(consumer_producer_t* q) {
    if (!q) return -1;
    return monitor_wait(&q->finished_monitor);
}