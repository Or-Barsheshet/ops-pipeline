#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"
#include <stdio.h>  

const char* plugin_transform(const char* input) {

    if (!input) return NULL;

    // printf("[DEBUG][flipper] input: %s\n", input);  

    if (strcmp(input, "<END>") == 0)
    return strdup("<END>");

    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) return NULL;

    // reverse the string
    for (size_t i = 0; i < len; ++i) {
        result[i] = input[len - 1 - i];
    }
    result[len] = '\0';

    // printf("[PLUGIN][flipper] transformed: %s\n", result);
    return result;
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "flipper", queue_size);
}