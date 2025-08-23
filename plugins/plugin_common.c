#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_common.h"
#include "sync/consumer_producer.h"

/* Global plugin context (one per .so) */
static plugin_context_t context;

/* Info log */
void log_info(plugin_context_t* context, const char* message) {
    if (context && message) {
        fprintf(stdout, "[INFO][%s] - %s\n", context->name, message);
        fflush(stdout);
    }
}

/* Error log */
void log_error(plugin_context_t* context, const char* message) {
    if (context && message) {
        fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
        fflush(stderr);
    }
}

/* Return plugin name */
const char* plugin_get_name(void) {
    return context.name;
}

/* Common initialization */
const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name,
                               int queue_size) {
    if (context.initialized) {
        return "Plugin already initialized";
    }
    if (!process_function || !name || queue_size <= 0) {
        return "Invalid plugin initialization arguments";
    }

    context.queue = malloc(sizeof(consumer_producer_t));
    if (!context.queue) {
        return "Failed to allocate queue";
    }

    if (consumer_producer_init(context.queue, queue_size) != 0) {
        return "Failed to initialize consumer-producer queue";
    }

    context.process_function = process_function;
    context.name = name;
    context.next_place_work = NULL;
    context.initialized = 1;
    context.finished = 0;

    if (pthread_create(&context.consumer_thread,
                       NULL,
                       plugin_consumer_thread,
                       &context) != 0) {
        return "Failed to create consumer thread";
    }

    return NULL; /* success */
}

/* Place work into queue */
const char* plugin_place_work(const char* str) {
    if (!context.initialized) return "Plugin not initialized";
    if (!str) return "Invalid input string";

    if (consumer_producer_put(context.queue, str) != 0) {
        return "Failed to enqueue item";
    }
    return NULL;
}

/* Attach next plugin */
void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (!context.initialized) {
        log_error(&context, "Attempted to attach before initialization");
        return;
    }
    context.next_place_work = next_place_work;
}

/* Wait until finished */
const char* plugin_wait_finished(void) {
    if (!context.initialized) return "Plugin not initialized";

    if (consumer_producer_wait_finished(context.queue) != 0) {
        return "Failed while waiting for queue to finish";
    }
    if (pthread_join(context.consumer_thread, NULL) != 0) {
        return "Failed to join consumer thread";
    }
    return NULL;
}

/* Finalize plugin */
const char* plugin_fini(void) {
    if (!context.initialized) return "Plugin not initialized";

    consumer_producer_destroy(context.queue);
    free(context.queue);
    context.queue = NULL;

    context.initialized = 0;
    context.finished = 0;
    context.name = NULL;
    context.process_function = NULL;
    context.next_place_work = NULL;

    return NULL;
}

/* Worker thread for each plugin */
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* ctx = (plugin_context_t*)arg;

    while (1) {
        char* input = consumer_producer_get(ctx->queue);
        if (!input) continue;

        if (strcmp(input, "<END>") == 0) {
            free(input);
            break;
        }

        const char* output = ctx->process_function(input);

        if (ctx->next_place_work) {
            ctx->next_place_work(output);
        } else {
            free((void*)output);
        }
        free(input);
    }

    if (ctx->next_place_work) {
        ctx->next_place_work("<END>");
    }
    consumer_producer_signal_finished(ctx->queue);
    ctx->finished = 1;
    return NULL;
}