#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "plugins/plugin_sdk.h"

// Function pointer typedefs
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void        (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);
typedef const char* (*plugin_get_name_func_t)(void);

// Plugin handle
typedef struct {
    void* handle;
    const char* name_for_logs;               // for messages before plugin_init returns a name
    plugin_get_name_func_t      get_name;
    plugin_init_func_t          init;
    plugin_fini_func_t          fini;
    plugin_place_work_func_t    place_work;
    plugin_attach_func_t        attach;
    plugin_wait_finished_func_t wait_finished;
} plugin_handle_t;

// Input reader thread args
typedef struct {
    plugin_place_work_func_t first_plugin_place_work;
} input_thread_args_t;

static void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Arguments:\n");
    printf("  queue_size    Positive integer for each plugin's queue capacity\n");
    printf("  plugin1..N    Names of plugins to load (without .so extension)\n");
    printf("\n");
    printf("Common plugins (if present):\n");
    printf("  logger, typewriter, uppercaser, rotator, flipper, expander\n");
    printf("\nExamples:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n");
}

#define LOAD_SYM(P,I,FIELD,SYMSTR) do {                           \
    dlerror();                                                     \
    *(void**)(&((P)[(I)].FIELD)) = dlsym((P)[(I)].handle, SYMSTR); \
    const char* _e = dlerror();                                    \
    if (_e) {                                                      \
        fprintf(stderr, "Error resolving %s for %s: %s\n",         \
                SYMSTR, plugin_names[(I)], _e);                    \
        print_usage();                                             \
        for (int j = 0; j <= (I); ++j)                             \
            if ((P)[j].handle) dlclose((P)[j].handle);             \
        free(P);                                                   \
        return 1;                                                  \
    }                                                              \
} while (0)

// Reject duplicate plugin names (we do NOT support multiple instances)
static int has_duplicate_names(char** names, int n, const char** dup_out) {
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (strcmp(names[i], names[j]) == 0) {
                if (dup_out) *dup_out = names[i];
                return 1;
            }
        }
    }
    return 0;
}

// Input reader thread — forwards lines to first plugin
static void* input_reader_thread(void* arg) {
    input_thread_args_t* args = (input_thread_args_t*)arg;
    char buffer[1025];
    int saw_end = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = 0; // strip trailing \n
        const char* err = args->first_plugin_place_work(buffer);
        if (err) fprintf(stderr, "[ERROR][input] %s\n", err);
        if (strcmp(buffer, "<END>") == 0) {
            saw_end = 1;
            break;
        }
    }
    if (!saw_end) {
        const char* err = args->first_plugin_place_work("<END>");
        if (err) fprintf(stderr, "[ERROR][input] %s\n", err);
    }
    free(args);
    return NULL;
}

int main(int argc, char* argv[]) {
    // ---- 1) Parse args ----
    if (argc < 3) {
        fprintf(stderr, "[ERROR] Not enough arguments.\n");
        print_usage();
        return 1;
    }
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "[ERROR] Queue size must be a positive integer.\n");
        print_usage();
        return 1;
    }
    int num_plugins = argc - 2;
    char** plugin_names = &argv[2];

    const char* dup_name = NULL;
    if (has_duplicate_names(plugin_names, num_plugins, &dup_name)) {
        fprintf(stderr, "[ERROR] Multiple instances of plugin '%s' are not supported in this implementation.\n", dup_name);
        fprintf(stderr, "Hint: The assignment allows single instance per plugin; multi-instance is a bonus.\n");
        return 1;
    }

    plugin_handle_t* plugins = (plugin_handle_t*)calloc(num_plugins, sizeof(plugin_handle_t));
    if (!plugins) {
        perror("Failed to allocate memory for plugins");
        return 1;
    }

    // ---- 2) dlopen + dlsym for each plugin (arbitrary name allowed if file exists) ----
    for (int i = 0; i < num_plugins; ++i) {
        char so_path[256];
        snprintf(so_path, sizeof(so_path), "./output/%s.so", plugin_names[i]);

        plugins[i].handle = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "[ERROR] loading plugin '%s' from %s: %s\n",
                    plugin_names[i], so_path, dlerror());
            print_usage();
            // cleanup previous
            for (int j = 0; j < i; ++j) {
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            return 1;
        }

        LOAD_SYM(plugins, i, get_name,      "plugin_get_name");
        LOAD_SYM(plugins, i, init,          "plugin_init");
        LOAD_SYM(plugins, i, fini,          "plugin_fini");
        LOAD_SYM(plugins, i, place_work,    "plugin_place_work");
        LOAD_SYM(plugins, i, attach,        "plugin_attach");
        LOAD_SYM(plugins, i, wait_finished, "plugin_wait_finished");

        // name for logs before init (get_name may rely on init in some impls)
        plugins[i].name_for_logs = plugin_names[i];
    }

    // ---- 3) init all plugins ----
    for (int i = 0; i < num_plugins; ++i) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "Error initializing plugin %s: %s\n",
                    plugins[i].name_for_logs, err);
            // best-effort cleanup
            for (int j = i - 1; j >= 0; --j) {
                if (plugins[j].fini) plugins[j].fini();
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            return 2;
        }
    }

    // ---- 4) attach chain (linear) ----
    for (int i = 0; i < num_plugins - 1; ++i) {
        plugins[i].attach(plugins[i + 1].place_work);
    }

    // ---- 5) input thread (so main can wait on plugins) ----
    pthread_t input_tid;
    input_thread_args_t* args = (input_thread_args_t*)malloc(sizeof(input_thread_args_t));
    if (!args) {
        perror("Failed to allocate memory for input thread args");
        for (int i = num_plugins - 1; i >= 0; --i) {
            if (plugins[i].fini) plugins[i].fini();
            if (plugins[i].handle) dlclose(plugins[i].handle);
        }
        free(plugins);
        return 1;
    }
    args->first_plugin_place_work = plugins[0].place_work;

    if (pthread_create(&input_tid, NULL, input_reader_thread, args) != 0) {
        perror("Failed to create input reader thread");
        free(args);
        for (int i = num_plugins - 1; i >= 0; --i) {
            if (plugins[i].fini) plugins[i].fini();
            if (plugins[i].handle) dlclose(plugins[i].handle);
        }
        free(plugins);
        return 1;
    }

    // ---- 6) wait_finished in ascending order ----
    for (int i = 0; i < num_plugins; ++i) {
        const char* err = plugins[i].wait_finished();
        if (err) {
            fprintf(stderr, "Error waiting for plugin %s: %s\n",
                    plugins[i].name_for_logs, err);
        }
    }

    // also wait for input thread
    pthread_join(input_tid, NULL);

    // ---- 7) fini + dlclose in reverse order ----
    for (int i = num_plugins - 1; i >= 0; --i) {
        if (plugins[i].fini) {
            const char* err = plugins[i].fini();
            if (err) {
                fprintf(stderr, "Error finalizing plugin %s: %s\n",
                        plugins[i].name_for_logs, err);
            }
        }
        if (plugins[i].handle) dlclose(plugins[i].handle);
    }

    free(plugins);
    printf("Pipeline shutdown complete\n");
    return 0;
}