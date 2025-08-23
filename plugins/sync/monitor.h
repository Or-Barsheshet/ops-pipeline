#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <time.h>

/* Monitor עם מצב "דביק" (flag signaled) */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  condition;
    int             signaled;
} monitor_t;

/* אתחול/השמדה */
int  monitor_init(monitor_t* monitor);
void monitor_destroy(monitor_t* monitor);

/* אות וסיפוס (איפוס) */
void monitor_signal(monitor_t* monitor);
void monitor_reset(monitor_t* monitor);

/* API 1: wait שמנהל את הנעילה בפנים */
int  monitor_wait(monitor_t* monitor);

/* API 2: wait שמניח שה‑mutex כבר נעול ע"י הקורא */
int  monitor_wait_locked(monitor_t* monitor);

/* API 3: כמו API 2, אבל עם timeout במילישניות.
   מחזיר 0 אם התקבל אות, 1 אם timeout, ‎-1 אם שגיאה. */
int  monitor_timedwait_locked(monitor_t* monitor, int milliseconds);

#endif /* MONITOR_H */