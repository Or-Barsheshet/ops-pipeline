#include <stdio.h>
#include <unistd.h>  // for usleep
#include <string.h>
#include "plugin_common.h"

const char* plugin_transform(const char* input) {

    if (!input) return NULL;
    printf("[DEBUG][typewrite] input: %s\n", input);  

    if (strcmp(input, "<END>") == 0){
        return strdup("<END>");
    }

    printf("[PLUGIN][typewriter] output: ");
    fflush(stdout);

    for (int i = 0; input[i]; ++i) {
        putchar(input[i]);
        fflush(stdout);
        usleep(100 * 1000);  // 100ms
    }

    putchar('\n');
    return strdup("<END>");  
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "typewriter", queue_size);
}