#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plugin_common.h"

const char* logger_transform(const char* input) {
    if (!input) return NULL;

    if (strcmp(input, "<END>") == 0) {
        return strdup("<END>"); 
    }
    
    printf("[logger] %s\n", input);
    return strdup(input);  // return a duplicate even if logger doesn't change it
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(logger_transform, "logger", queue_size);
}
