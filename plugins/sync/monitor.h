#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <time.h>

/* Monitor structure (sticky flag for lost signals protection) */
typedef struct {
    pthread_cond_t condition;
    int            signaled;
} monitor_t;

int  monitor_init(monitor_t* m);
void monitor_destroy(monitor_t* m);

void monitor_signal_locked(monitor_t* m, pthread_mutex_t* external_mutex);
void monitor_reset_locked(monitor_t* m, pthread_mutex_t* external_mutex);

int  monitor_wait_locked(monitor_t* m, pthread_mutex_t* external_mutex);

#endif /* MONITOR_H */