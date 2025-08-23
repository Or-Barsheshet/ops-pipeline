#include "monitor.h"
#include <errno.h>

int monitor_init(monitor_t* m) {
    if (!m) return -1;
    if (pthread_cond_init(&m->condition, NULL) != 0) return -1;
    m->signaled = 0;
    return 0;
}

void monitor_destroy(monitor_t* m) {
    if (!m) return;
    pthread_cond_destroy(&m->condition);
}

void monitor_signal_locked(monitor_t* m, pthread_mutex_t* external_mutex) {
    if (!m || !external_mutex) return;
    m->signaled = 1;
    pthread_cond_signal(&m->condition);
}

void monitor_reset_locked(monitor_t* m, pthread_mutex_t* external_mutex) {
    if (!m || !external_mutex) return;
    m->signaled = 0;
}

int monitor_wait_locked(monitor_t* m, pthread_mutex_t* external_mutex) {
    if (!m || !external_mutex) return -1;
    while (!m->signaled) {
        if (pthread_cond_wait(&m->condition, external_mutex) != 0)
            return -1;
    }
    m->signaled = 0;
    return 0;
}