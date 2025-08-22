#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "plugin_common.h"

// plugin-specific transformation logic
const char* plugin_transform(const char* input) {

    if (!input) return NULL;

    if (strcmp(input, "<END>") == 0){
        return strdup("<END>");
    }

    // allocate a copy of the input
    char* result = strdup(input);
    if (!result) return NULL;

    // convert each character to uppercase
    for (int i = 0; result[i]; ++i) {
        result[i] = toupper(result[i]);
    }
    
    return result;
}

// plugin initialization — uses shared common logic
const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "uppercaser", queue_size);
}