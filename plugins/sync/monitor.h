#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <time.h>

// sticky monitor state; caller holds an external mutex
typedef struct {
    pthread_cond_t condition;   // condition variable
    int            signaled;    // sticky flag to avoid lost signals
} monitor_t;

int  monitor_init(monitor_t* m);
void monitor_destroy(monitor_t* m);

// caller must hold external_mutex while calling these
void monitor_signal_locked(monitor_t* m, pthread_mutex_t* external_mutex);
void monitor_reset_locked(monitor_t* m, pthread_mutex_t* external_mutex);

// waits until signaled; caller must hold external_mutex on entry
int  monitor_wait_locked(monitor_t* m, pthread_mutex_t* external_mutex);

#endif // MONITOR_H