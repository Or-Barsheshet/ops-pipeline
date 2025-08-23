#define _GNU_SOURCE
#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "plugins/plugin_sdk.h"

// Define function pointer types for clarity
typedef const char* (*plugin_init_func_t)(int);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char*);
typedef void (*plugin_attach_func_t)(const char* (*)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);
typedef const char* (*plugin_get_name_func_t)(void);

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

// A struct to hold all the function pointers and data for a loaded plugin
typedef struct {
    void* handle;
    const char* name;
    plugin_get_name_func_t get_name;
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    char temp_path[256];
} plugin_handle_t;

// Struct to pass arguments to the input reader thread
typedef struct {
    plugin_place_work_func_t first_plugin_place_work;
} input_thread_args_t;

static void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Arguments:\n");
    printf("  queue_size    Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N    Names of plugins to load (without .so extension)\n");
    printf("Available plugins:\n");
    printf("  logger     - Logs all strings that pass through\n");
    printf("  typewriter - Simulates typewriter effect with delays\n");
    printf("  uppercaser - Converts strings to uppercase\n");
    printf("  rotator    - Moves every character to the right. Last character moves to the beginning.\n");
    printf("  flipper    - Reverses the order of characters\n");
    printf("  expander   - Expands each character with spaces\n");
    printf("Example:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
    printf("\n");
    printf("  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n");
}

// This function runs in a dedicated thread to read from stdin
void* input_reader_thread(void* arg) {
    input_thread_args_t* args = (input_thread_args_t*)arg;
    char buffer[1025];
    int saw_end = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\n")] = 0;

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
static int count_previous(const char* name, char** names, int upto) {
    int c = 0;
    for (int k = 0; k < upto; ++k)
        if (strcmp(names[k], name) == 0) c++;
    return c;
}

static int copy_file(const char* src, const char* dst) {
    FILE *in = fopen(src, "rb");
    if (!in) return -1;
    FILE *out = fopen(dst, "wb");
    if (!out) { fclose(in); return -1; }
    char buf[1<<16];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { fclose(in); fclose(out); return -1; }
    }
    fclose(in);
    fclose(out);
    return 0;
}


int main(int argc, char* argv[]) {
    // 1. Argument Parsing
    if (argc < 3) {
        fprintf(stderr, "Error: Not enough arguments.\n");
        print_usage();
        return 1;
    }
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "Error: Queue size must be a positive integer.\n");
        print_usage();
        return 1;
    }
    int num_plugins = argc - 2;
    char** plugin_names = &argv[2];

    plugin_handle_t* plugins = calloc(num_plugins, sizeof(plugin_handle_t));
    if (!plugins) {
        perror("Failed to allocate memory for plugins");
        return 1;
    }

    // 2. Load Plugin Shared Objects
   // 2. Load Plugin Shared Objects
for (int i = 0; i < num_plugins; i++) {
    char plugin_path[256];
    snprintf(plugin_path, sizeof(plugin_path), "./output/%s.so", plugin_names[i]);

    // בדיקה: האם זה מופע שני/שלישי של אותו פלאגין?
    int inst = count_previous(plugin_names[i], plugin_names, i);
    const char* path_to_load = plugin_path;
    plugins[i].temp_path[0] = '\0';

    if (inst > 0) {
        // יוצרים עותק ייחודי כדי לקבל BSS/סטטיקים נפרדים לכל מופע
        snprintf(plugins[i].temp_path, sizeof(plugins[i].temp_path),
                 "./output/%s__inst%d.so", plugin_names[i], inst);
        if (copy_file(plugin_path, plugins[i].temp_path) != 0) {
            fprintf(stderr, "Error: failed to create copy for %s (instance %d)\n",
                    plugin_names[i], inst);
            print_usage();
            for (int j = 0; j < i; ++j) if (plugins[j].handle) dlclose(plugins[j].handle);
            free(plugins);
            return 1;
        }
        path_to_load = plugins[i].temp_path;
    }

    // נשארים עם dlopen בלבד, כדרישת המטלה
    plugins[i].handle = dlopen(path_to_load, RTLD_NOW | RTLD_LOCAL);
    if (!plugins[i].handle) {
        fprintf(stderr, "Error loading plugin %s: %s\n", plugin_names[i], dlerror());
        print_usage();
        for (int j = 0; j < i; j++) if (plugins[j].handle) dlclose(plugins[j].handle);
        // ניקוי קבצים זמניים שנוצרו עד כה
        for (int j = 0; j < i; ++j) if (plugins[j].temp_path[0]) unlink(plugins[j].temp_path);
        free(plugins);
        return 1;
    }

    LOAD_SYM(plugins, i, get_name,      "plugin_get_name");
    LOAD_SYM(plugins, i, init,          "plugin_init");
    LOAD_SYM(plugins, i, fini,          "plugin_fini");
    LOAD_SYM(plugins, i, place_work,    "plugin_place_work");
    LOAD_SYM(plugins, i, attach,        "plugin_attach");
    LOAD_SYM(plugins, i, wait_finished, "plugin_wait_finished");

    // שם ללוגים לפני init (plugin_get_name יחזיר ערך רק אחרי init אצלך)
    plugins[i].name = plugin_names[i];
        
    }

    // 3. Initialize Plugins
    for (int i = 0; i < num_plugins; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err) {
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, err);
            for (int j = i-1; j >= 0; --j){
                if(plugins[j].fini) plugins[j].fini();
                if (plugins[j].handle) dlclose(plugins[j].handle);
            }
            free(plugins);
            return 2;
        }
    }

    // 4. Attach Plugins Together
    for (int i = 0; i < num_plugins - 1; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }

    // 5. Start a dedicated thread for reading input to prevent main from blocking
    pthread_t input_tid;
    input_thread_args_t* thread_args = malloc(sizeof(input_thread_args_t));
    if(!thread_args){
        perror("Failed to allocate memory for input thread arguments");
        for (int i = 0; i < num_plugins; i++) dlclose(plugins[i].handle);
        free(plugins);
        return 1;
    }
    thread_args->first_plugin_place_work = plugins[0].place_work;
    if (pthread_create(&input_tid, NULL, input_reader_thread, thread_args) != 0) {
        perror("Failed to create input reader thread");
        for (int i = 0; i < num_plugins; i++) dlclose(plugins[i].handle);
        free(plugins);
        return 1;
    }

    // 6. Wait for Plugins to Finish 
    for (int i = 0; i < num_plugins; i++) {
       const char* err = plugins[i].wait_finished();
       if(err){
        fprintf(stderr, "Error waiting for plugin %s: %s\n", plugins[i].name, err);
       }
    }
    
    // Wait for the input thread to finish its work as well
    pthread_join(input_tid, NULL);

    // 7. Cleanup
    for (int i = num_plugins - 1; i >= 0; i--) {
        plugins[i].fini();
        dlclose(plugins[i].handle);
        if (plugins[i].temp_path[0]) {
            unlink(plugins[i].temp_path);  // מחיקת העותק הזמני
        }   
    }
    
    free(plugins);
    printf("Pipeline shutdown complete\n");
    return 0;
}