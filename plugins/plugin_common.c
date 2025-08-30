#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "plugin_common.h"
#include "sync/consumer_producer.h"

// single context instance per shared object 
static plugin_context_t g_ctx;

// info to stdout (non-fatal) 
void log_info(plugin_context_t* ctx, const char* msg) {
    if (ctx && msg) {
        fprintf(stdout, "[info][%s] %s\n", ctx->name, msg);
        fflush(stdout);
    }
}

// errors to stderr (fatal/non-fatal) 
void log_error(plugin_context_t* ctx, const char* msg) {
    if (ctx && msg) {
        fprintf(stderr, "[ERROR][%s] %s\n", ctx->name, msg);
        fflush(stderr);
    }
}

// export name for external use 
const char* plugin_get_name(void) {
    return g_ctx.name;
}

// shared init used by plugins to bind their transform fn 
const char* common_plugin_init(const char* (*process_function)(const char*),
                               const char* name,
                               int queue_size) {
    if (g_ctx.is_init) {
        return "already initialized";
    }
    if (!process_function || !name || queue_size <= 0) {
        return "invalid init args";
    }

    g_ctx.q = (consumer_producer_t*)malloc(sizeof(consumer_producer_t));
    if (!g_ctx.q) {
        return "queue alloc failed";
    }

    if (consumer_producer_init(g_ctx.q, queue_size) != 0) {
        return "queue init failed";
    }

    g_ctx.transform = process_function;
    g_ctx.name = name;
    g_ctx.send_next = NULL;
    g_ctx.is_init = 1;
    g_ctx.is_done = 0;

    if (pthread_create(&g_ctx.worker_tid, NULL, plugin_consumer_thread, &g_ctx) != 0) {
        return "consumer thread create failed";
    }

    return NULL; /* success */
}

// enqueue input for this plugin 
const char* plugin_place_work(const char* str) {
    if (!g_ctx.is_init) return "plugin not initialized";
    if (!str) return "null input";

    if (consumer_producer_put(g_ctx.q, str) != 0) {
        return "enqueue failed";
    }
    return NULL;
}

// set the next stage callback 
void plugin_attach(const char* (*next_place_work)(const char*)) {
    if (!g_ctx.is_init) {
        log_error(&g_ctx, "attach before init");
        return;
    }
    g_ctx.send_next = next_place_work;
}

// wait until this plugin finishes draining 
const char* plugin_wait_finished(void) {
    if (!g_ctx.is_init) return "plugin not initialized";

    if (consumer_producer_wait_finished(g_ctx.q) != 0) {
        return "wait finished failed";
    }
    if (pthread_join(g_ctx.worker_tid, NULL) != 0) {
        return "join failed";
    }
    return NULL;
}

// finalize and release resources 
const char* plugin_fini(void) {
    if (!g_ctx.is_init) return "plugin not initialized";

    consumer_producer_destroy(g_ctx.q);
    free(g_ctx.q);
    g_ctx.q = NULL;

    g_ctx.is_init = 0;
    g_ctx.is_done = 0;
    g_ctx.name = NULL;
    g_ctx.transform = NULL;
    g_ctx.send_next = NULL;

    return NULL;
}

// internal helper: check end sentinel 
static inline int is_end(const char* s) {
    return s && strcmp(s, "<END>") == 0;
}

// worker thread: consumes, transforms, forwards 
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* ctx = (plugin_context_t*)arg;

    for (;;) {
        char* in = consumer_producer_get(ctx->q);
        if (!in) continue;

        if (is_end(in)) {
            free(in);
            break;
        }

        const char* out = ctx->transform(in);
        if (ctx->send_next) {
            (void)ctx->send_next(out);
        } else {
            free((void*)out);
        }
        free(in);
    }

    if (ctx->send_next) {
        (void)ctx->send_next("<END>");
    }
    consumer_producer_signal_finished(ctx->q);
    ctx->is_done = 1;
    return NULL;
}