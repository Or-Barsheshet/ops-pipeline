#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include <pthread.h>
#include "sync/consumer_producer.h"

typedef struct {
    const char* name;
    consumer_producer_t* queue;
    pthread_t consumer_thread;
    const char* (*next_place_work)(const char*);
    const char* (*process_function)(const char*);
    int initialized;
    int finished;
} plugin_context_t;

void* plugin_consumer_thread(void* arg);

void log_error(plugin_context_t* context, const char* message);
void log_info(plugin_context_t* context, const char* message);

__attribute__((visibility("default")))
const char* plugin_get_name(void);

const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name, int queue_size);

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