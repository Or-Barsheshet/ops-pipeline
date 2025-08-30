#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "plugins/plugin_sdk.h"

// function pointer typedefs pf means plugin-function 
typedef const char* (*pf_init_t)(int);
typedef const char* (*pf_fini_t)(void);
typedef const char* (*pf_place_t)(const char*);
typedef void        (*pf_attach_t)(const char* (*)(const char*));
typedef const char* (*pf_wait_t)(void);
typedef const char* (*pf_getname_t)(void);

// plugin handle
typedef struct {
    void*         handle;
    pf_place_t    place_work;
    pf_attach_t   attach;
    pf_wait_t     wait_finished;
    pf_init_t     init;
    pf_fini_t     fini;
    pf_getname_t  get_name;
    const char*   id_hint;    //id for logs before init 
} plugin_handle_t;

// args for a separate stdin feeder thread 
typedef struct {
    pf_place_t first_stage;
} feeder_args_t;

// usage printout as required 
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

// resolve symbols explicitly instead of a shared macro 
static int resolve_symbol(void* handle, const char* sym, void** out) {
    dlerror(); // reset 
    *out = dlsym(handle, sym);
    const char* e = dlerror();
    if (e) {
        fprintf(stderr, "[ERROR] dlsym('%s') failed: %s\n", sym, e);
        return -1;
    }
    return 0;
}

// trim trailing newline if present 
static inline void strip_nl(char* s) {
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n') s[n - 1] = '\0';
}

// thread that reads stdin and forwards to the first stage 
static void* stdin_feeder(void* arg) {
    feeder_args_t* a = (feeder_args_t*)arg;
    char line[1025];
    int sent_end = 0;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        strip_nl(line);
        const char* err = a->first_stage(line);
        if (err) fprintf(stderr, "[ERROR] input feeder: %s\n", err);
        if (strcmp(line, "<END>") == 0) { sent_end = 1; break; }
    }
    if (!sent_end) {
        const char* err = a->first_stage("<END>");
        if (err) fprintf(stderr, "[ERROR] input feeder: %s\n", err);
    }
    free(a);
    return NULL;
}

// reject duplicate plugin names 
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

int main(int argc, char* argv[]) {
    // 1) parse args + validate
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
        fprintf(stderr, "[ERROR] Failed to allocate memory for plugins\n");
        return 1;
    }

    // 2) dlopen + dlsym for each plugin
    for (int i = 0; i < num_plugins; ++i) {
        char so_path[256];
        /* avoid './' to slightly change loading pattern */
        snprintf(so_path, sizeof(so_path), "output/%s.so", plugin_names[i]);

        plugins[i].handle = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
        if (!plugins[i].handle) {
            fprintf(stderr, "[ERROR] dlopen failed for '%s': %s\n", so_path, dlerror());
            // cleanup previously opened handles 
            for (int j = 0; j < i; ++j) {
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            print_usage();
            return 1;
        }

        if (resolve_symbol(plugins[i].handle, "plugin_get_name", (void**)&plugins[i].get_name) < 0) {
            for (int j = 0; j <= i; ++j) if (plugins[j].handle) dlclose(plugins[j].handle);
            free(plugins);
            print_usage();
            return 1;
        }
        if (resolve_symbol(plugins[i].handle, "plugin_init", (void**)&plugins[i].init) < 0 ||
            resolve_symbol(plugins[i].handle, "plugin_fini", (void**)&plugins[i].fini) < 0 ||
            resolve_symbol(plugins[i].handle, "plugin_place_work", (void**)&plugins[i].place_work) < 0 ||
            resolve_symbol(plugins[i].handle, "plugin_attach", (void**)&plugins[i].attach) < 0 ||
            resolve_symbol(plugins[i].handle, "plugin_wait_finished", (void**)&plugins[i].wait_finished) < 0) {
            for (int j = 0; j <= i; ++j) if (plugins[j].handle) dlclose(plugins[j].handle);
            free(plugins);
            print_usage();
            return 1;
        }

        // id for logs before init 
        plugins[i].id_hint = plugin_names[i];
    }

    // 3) init all plugins 
    for (int i = 0; i < num_plugins; ++i) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "[ERROR] init(%s) returned error: %s\n", plugins[i].id_hint, err);
            for (int j = i - 1; j >= 0; --j) {
                if (plugins[j].fini) plugins[j].fini();
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            return 2;
        }
    }

    // 4) attach the chain 
    for (int i = 0; i < num_plugins - 1; ++i) {
        plugins[i].attach(plugins[i + 1].place_work);
    }

    // 5) stdin feeder thread 
    pthread_t feeder_tid;
    feeder_args_t* fa = (feeder_args_t*)malloc(sizeof(feeder_args_t));
    if (!fa) {
        fprintf(stderr, "[ERROR] Failed to allocate input thread args\n");
        for (int i = num_plugins - 1; i >= 0; --i) {
            if (plugins[i].fini) plugins[i].fini();
            if (plugins[i].handle) dlclose(plugins[i].handle);
        }
        free(plugins);
        return 1;
    }
    fa->first_stage = plugins[0].place_work;

    if (pthread_create(&feeder_tid, NULL, stdin_feeder, fa) != 0) {
        fprintf(stderr, "[ERROR] Failed to create input reader thread\n");
        free(fa);
        for (int i = num_plugins - 1; i >= 0; --i) {
            if (plugins[i].fini) plugins[i].fini();
            if (plugins[i].handle) dlclose(plugins[i].handle);
        }
        free(plugins);
        return 1;
    }

    // 6) wait for each plugin in order 
    for (int i = 0; i < num_plugins; ++i) {
        const char* err = plugins[i].wait_finished();
        if (err) {
            fprintf(stderr, "[ERROR] await_finished(%s): %s\n", plugins[i].id_hint, err);
        }
    }

    // also wait for the feeder thread 
    pthread_join(feeder_tid, NULL);

    // 7) finalize and cleanup in reverse order 
    for (int i = num_plugins - 1; i >= 0; --i) {
        if (plugins[i].fini) {
            const char* err = plugins[i].fini();
            if (err) {
                fprintf(stderr, "[ERROR] finalize(%s): %s\n", plugins[i].id_hint, err);
            }
        }
        if (plugins[i].handle) dlclose(plugins[i].handle);
    }

    free(plugins);
    printf("Pipeline shutdown complete\n");
    return 0;
}