#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_common.h"
#include "sync/consumer_producer.h"

static plugin_context_t context; // plugin context to hold state and configuration

//prints info message 
void log_info(plugin_context_t* context, const char* message) {
    if (context && message) {
        fprintf(stdout, "[INFO][%s] - %s\n", context->name, message);
        fflush(stdout);
    }
}
//prints error message 
void log_error(plugin_context_t* context, const char* message) {
    if (context && message) {
        fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
        fflush(stderr);
    }
}

const char* plugin_get_name(void) {
    return context.name;
}

const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name,
                               int queue_size) {
    // prevent double init
    if (context.initialized) {
        return "Plugin already initialized";
    }

    // validate input args
    if (!process_function || !name || queue_size <= 0) {
        return "Invalid plugin initialization arguments";
    }

    // allocate memory 
    context.queue = malloc(sizeof(consumer_producer_t));
    if (!context.queue) {
        return "Failed to allocate queue";
    }

    // initialize the producer-consumer queue
    const char* err = consumer_producer_init(context.queue, queue_size);
    if (err != NULL) {
        return err;  // forward error message from the queue init
    }

    // store function pointers and metadata
    context.process_function = process_function;
    context.name = name;
    context.next_place_work = NULL;
    context.initialized = 1;
    context.finished = 0;

    // create the worker thread that processes strings from the queue
    if (pthread_create(&context.consumer_thread, NULL, plugin_consumer_thread, &context) != 0) {
        return "Failed to create consumer thread";
    }

    return NULL; // success
}

const char* plugin_place_work(const char* str) {
    // check that the plugin has been initialized
    if (!context.initialized) {
        return "Plugin not initialized";
    }

    // validate input 
    if (!str) {
        return "Invalid input string";
    }

    // put the string into the queue
    const char* err = consumer_producer_put(context.queue, str);
    if (err != NULL) {
        return err;  // forward queue error message
    }

    return NULL; // success
}

void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (!context.initialized) {
        log_error(&context, "Attempted to attach before initialization");
        return;
    }

    context.next_place_work = next_place_work;
}

const char* plugin_wait_finished(void) {
    if (!context.initialized) {
        return "Plugin not initialized";
    }

    // Wait for the queue to be fully processed
    if (consumer_producer_wait_finished(context.queue) != 0) {
        return "Failed while waiting for queue to finish";
    }

    // Wait for the thread to finish execution
    if (pthread_join(context.consumer_thread, NULL) != 0) {
        return "Failed to join consumer thread";
    }

    return NULL; // Success
}


const char* plugin_fini(void) {
    if (!context.initialized) {
        return "Plugin not initialized";
    }

    // clean up queue
    consumer_producer_destroy(context.queue);
    free(context.queue);
    context.queue = NULL;

    // reset plugin state
    context.initialized = 0;
    context.finished = 0;
    context.name = NULL;
    context.process_function = NULL;
    context.next_place_work = NULL;

    return NULL; // success
}


void* plugin_consumer_thread(void* arg) {
    plugin_context_t* ctx = (plugin_context_t*)arg;

    while (1) {
        // get a string from the queue (blocks if empty)
        char* input = consumer_producer_get(ctx->queue);
        if (!input) {
            continue; 
        }

        // check for shutdown signal
        if (strcmp(input, "<END>") == 0) {
            free(input);
            break;
        }

        // process the string
        const char* output = ctx->process_function(input);

        // send to the next plugin in the chain, if exists
        if (ctx->next_place_work) {
            ctx->next_place_work(output);
        } else {
            // if this is the last plugin, free the processed string
            free((void*)output);
        }

        // free the original input 
        free(input);
    }

    // forward <END> to the next plugin (if exists)
    if (ctx->next_place_work) {
        ctx->next_place_work("<END>");
    }

    // signal that this plugin has finished 
    consumer_producer_signal_finished(ctx->queue);
    ctx->finished = 1;

    return NULL;
}