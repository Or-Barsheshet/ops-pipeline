#include "monitor.h"
#include <errno.h>   // ETIMEDOUT

/* מחשב זמן absolute לעוד ms מילישניות.
   משתמש ב‑C11 timespec_get במקום clock_gettime. */
static void timespec_after_ms(struct timespec* ts, int ms) {
    struct timespec now;
    timespec_get(&now, TIME_UTC);               // C11, ללא תלות ב‑POSIX
    ts->tv_sec  = now.tv_sec + (ms / 1000);
    long nsec   = now.tv_nsec + (long)(ms % 1000) * 1000000L;
    ts->tv_sec += nsec / 1000000000L;
    ts->tv_nsec = nsec % 1000000000L;
}

int monitor_init(monitor_t* m) {
    if (!m) return -1;
    if (pthread_mutex_init(&m->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&m->condition, NULL) != 0) {
        pthread_mutex_destroy(&m->mutex);
        return -1;
    }
    m->signaled = 0;
    return 0;
}

void monitor_destroy(monitor_t* m) {
    if (!m) return;
    pthread_mutex_destroy(&m->mutex);
    pthread_cond_destroy(&m->condition);
}

void monitor_signal(monitor_t* m) {
    if (!m) return;
    pthread_mutex_lock(&m->mutex);
    m->signaled = 1;
    pthread_cond_signal(&m->condition);   // מעיר ממתין אחד (מספיק כאן)
    pthread_mutex_unlock(&m->mutex);
}

void monitor_reset(monitor_t* m) {
    if (!m) return;
    pthread_mutex_lock(&m->mutex);
    m->signaled = 0;
    pthread_mutex_unlock(&m->mutex);
}

int monitor_wait(monitor_t* m) {
    if (!m) return -1;
    pthread_mutex_lock(&m->mutex);
    while (!m->signaled) {
        pthread_cond_wait(&m->condition, &m->mutex);
    }
    m->signaled = 0;                       // צרכנו את האות
    pthread_mutex_unlock(&m->mutex);
    return 0;
}

/* מניח: המנעול כבר נעול ע"י הקורא */
int monitor_wait_locked(monitor_t* m) {
    if (!m) return -1;
    while (!m->signaled) {
        pthread_cond_wait(&m->condition, &m->mutex);
    }
    m->signaled = 0;
    return 0;
}

/* מניח: המנעול כבר נעול ע"י הקורא; ממתין עד timeout */
int monitor_timedwait_locked(monitor_t* m, int milliseconds) {
    if (!m) return -1;

    if (m->signaled) {          // אות "דביק" שכבר הגיע קודם
        m->signaled = 0;
        return 0;
    }

    struct timespec ts;
    timespec_after_ms(&ts, milliseconds);

    int rc;
    while (!m->signaled) {
        rc = pthread_cond_timedwait(&m->condition, &m->mutex, &ts);
        if (rc == ETIMEDOUT) return 1;     // לא הגיע אות עד הדדליין
        if (rc != 0)        return -1;     // שגיאה אחרת
    }
    m->signaled = 0;
    return 0;
}