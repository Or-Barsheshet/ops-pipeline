#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <pthread.h>
#include "sync/consumer_producer.h"

// shared plugin context 
typedef struct {
    const char* name;                              /* plugin display name */
    consumer_producer_t* q;                        /* input queue (heap) */
    pthread_t worker_tid;                          /* consumer thread id */
    const char* (*send_next)(const char*);         /* next stage place_work */
    const char* (*transform)(const char*);         /* plugin transform fn */
    int is_init;                                   /* init state flag */
    int is_done;                                   /* finished flag */
} plugin_context_t;

// worker entry 
void* plugin_consumer_thread(void* arg);

// logging helpers 
void log_error(plugin_context_t* ctx, const char* msg);
void log_info(plugin_context_t* ctx, const char* msg);

// plugin api 
__attribute__((visibility("default")))
const char* plugin_get_name(void);

const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name,
                               int queue_size);

__attribute__((visibility("default")))
const char* plugin_init(int queue_size);

__attribute__((visibility("default")))
const char* plugin_fini(void);

__attribute__((visibility("default")))
const char* plugin_place_work(const char* str);

__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*));

__attribute__((visibility("default")))
const char* plugin_wait_finished(void);

#endif 