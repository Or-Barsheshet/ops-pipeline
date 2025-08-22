#include "monitor.h"

// initialize the monitor: set mutex, condition, and reset signaled flag
int monitor_init(monitor_t* monitor) {
    if (!monitor) return -1;

    // initialize mutex
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) return -1;

    // initialize condition variable
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex);
        return -1;
    }

    // reset signaled flag
    monitor->signaled = 0;
    return 0;
}

// destroy the monitor: release mutex and condition resources
void monitor_destroy(monitor_t* monitor) {
    if (!monitor) return;

    pthread_mutex_destroy(&monitor->mutex);
    pthread_cond_destroy(&monitor->condition);
}

// signal the monitor: set flag and wake one waiting thread
void monitor_signal(monitor_t* monitor) {
    if (!monitor) return;

    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 1;
    pthread_cond_signal(&monitor->condition); // wake one
    pthread_mutex_unlock(&monitor->mutex);
}

// reset the monitor's signaled flag to 0
void monitor_reset(monitor_t* monitor) {
    if (!monitor) return;

    pthread_mutex_lock(&monitor->mutex);
    monitor->signaled = 0;
    pthread_mutex_unlock(&monitor->mutex);
}

// wait until monitor is signaled (blocking)
int monitor_wait(monitor_t* monitor) {
    if (!monitor) return -1;

    while (!monitor->signaled) {
        pthread_cond_wait(&monitor->condition, &monitor->mutex);
    }
    monitor->signaled = 0;
    return 0;
}