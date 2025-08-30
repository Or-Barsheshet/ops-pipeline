#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plugin_common.h"

// print and forward the line unchanged
const char* logger_transform(const char* input_str) {
    if (!input_str) return NULL;

    if (strcmp(input_str, "<END>") == 0) {
        return strdup("<END>"); 
    }
    
    printf("[logger] %s\n", input_str);
    fflush(stdout);
    return strdup(input_str);  
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(logger_transform, "logger", queue_size);
}
