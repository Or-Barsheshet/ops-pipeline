#include <stdlib.h>
#include <string.h>
#include "consumer_producer.h"

const char* consumer_producer_init(consumer_producer_t* queue, int capacity) {
    if (!queue || capacity <= 0) {
        return "Invalid queue or capacity";
    }

    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    queue->items = (char**) malloc(sizeof(char*) * capacity);
    if (!queue->items) {
        return "Failed to allocate queue items";
    }

    if (monitor_init(&queue->not_full_monitor) != 0) {
        free(queue->items);
        return "Failed to initialize not_full monitor";
    }

    if (monitor_init(&queue->not_empty_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Failed to initialize not_empty monitor";
    }

    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Failed to initialize finished monitor";
    }

    return NULL; // success
}

void consumer_producer_destroy(consumer_producer_t* queue) {
    if (!queue) return;

    // destroy monitors
    monitor_destroy(&queue->not_full_monitor);
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);

    // free item buffer
    free(queue->items);
    queue->items = NULL;

    // reset fields (optional safety)
    queue->capacity = 0;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
}


const char* consumer_producer_put(consumer_producer_t* queue, const char* item) {
    if (!queue || !item) {
        return "Invalid queue or item";
    }

    // wait until there is space in the queue
    pthread_mutex_lock(&queue->not_full_monitor.mutex);
    while (queue->count == queue->capacity) {
        monitor_wait(&queue->not_full_monitor);
    }

    // copy the string into the queue
    queue->items[queue->tail] = strdup(item);
    if (!queue->items[queue->tail]) {
        pthread_mutex_unlock(&queue->not_full_monitor.mutex);
        return "Failed to allocate memory for item";
    }

    // update tail and count
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    // signal to consumers that something is available
    monitor_signal(&queue->not_empty_monitor);

    pthread_mutex_unlock(&queue->not_full_monitor.mutex);
    return NULL; // success
}

char* consumer_producer_get(consumer_producer_t* queue) {
    if (!queue) return NULL;

    // wait until there is at least one item in the queue
    pthread_mutex_lock(&queue->not_empty_monitor.mutex);
    while (queue->count == 0) {
        monitor_wait(&queue->not_empty_monitor);
    }

    // get the item from the queue
    char* item = queue->items[queue->head];
    queue->items[queue->head] = NULL; // optional: clear pointer
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    // signal to producers that space is available
    monitor_signal(&queue->not_full_monitor);

    pthread_mutex_unlock(&queue->not_empty_monitor.mutex);
    return item;
}

void consumer_producer_signal_finished(consumer_producer_t* queue) {
    if (!queue) return;

    monitor_signal(&queue->finished_monitor);
}

int consumer_producer_wait_finished(consumer_producer_t* queue) {
    if (!queue) return -1;

    return monitor_wait(&queue->finished_monitor);
}