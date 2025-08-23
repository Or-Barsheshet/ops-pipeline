#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_common.h"
#include "sync/consumer_producer.h"

static plugin_context_t context;

void log_info(plugin_context_t* ctx, const char* msg) {
    if (ctx && msg) { fprintf(stdout, "[INFO][%s] - %s\n", ctx->name, msg); fflush(stdout); }
}
void log_error(plugin_context_t* ctx, const char* msg) {
    if (ctx && msg) { fprintf(stderr, "[ERROR][%s] - %s\n", ctx->name, msg); fflush(stderr); }
}

const char* plugin_get_name(void) { return context.name; }

const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name, int queue_size)
{
    if (context.initialized) return "Plugin already initialized";
    if (!process_function || !name || queue_size <= 0) return "Invalid plugin initialization arguments";

    context.queue = (consumer_producer_t*)malloc(sizeof(consumer_producer_t));
    if (!context.queue) return "Failed to allocate queue";

    int rc = consumer_producer_init(context.queue, queue_size);
    if (rc != 0) {
        free(context.queue);
        context.queue = NULL;
        return "Queue init failed";
    }

    context.process_function = process_function;
    context.name             = name;
    context.next_place_work  = NULL;
    context.initialized      = 1;
    context.finished         = 0;

    if (pthread_create(&context.consumer_thread, NULL, plugin_consumer_thread, &context) != 0) {
        consumer_producer_destroy(context.queue);
        free(context.queue);
        context.queue = NULL;
        context.initialized = 0;
        return "Failed to create consumer thread";
    }
    return NULL;
}

const char* plugin_place_work(const char* str) {
    if (!context.initialized) return "Plugin not initialized";
    if (!str) return "Invalid input string";

    int rc = consumer_producer_put(context.queue, str);
    if (rc != 0) return "Queue put failed";
    return NULL;
}

void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (!context.initialized) {
        log_error(&context, "Attempted to attach before initialization");
        return;
    }
    context.next_place_work = next_place_work;
}

const char* plugin_wait_finished(void) {
    if (!context.initialized) return "Plugin not initialized";
    if (consumer_producer_wait_finished(context.queue) != 0) return "Wait finished failed";
    if (pthread_join(context.consumer_thread, NULL) != 0) return "Join failed";
    return NULL;
}

const char* plugin_fini(void) {
    if (!context.initialized) return "Plugin not initialized";

    consumer_producer_destroy(context.queue);
    free(context.queue);
    context.queue = NULL;

    context.initialized     = 0;
    context.finished        = 0;
    context.name            = NULL;
    context.process_function= NULL;
    context.next_place_work = NULL;

    return NULL;
}

void* plugin_consumer_thread(void* arg) {
    plugin_context_t* ctx = (plugin_context_t*)arg;

    for (;;) {
        char* input = consumer_producer_get(ctx->queue);
        if (input == NULL) {
            if (ctx->next_place_work) ctx->next_place_work("<END>");
            consumer_producer_signal_finished(ctx->queue);
            ctx->finished = 1;
            return NULL;
        }

        if (strcmp(input, "<END>") == 0) {
            free(input);
            if (ctx->next_place_work) ctx->next_place_work("<END>");
            consumer_producer_signal_finished(ctx->queue);
            ctx->finished = 1;
            return NULL;
        }

        const char* output = ctx->process_function(input);

        if (ctx->next_place_work) {
            ctx->next_place_work(output);
        } else {
            free((void*)output);
        }
        free(input);
    }
}