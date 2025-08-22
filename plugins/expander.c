#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

const char* plugin_transform(const char* input) {
    if (!input) return NULL;

    printf("[DEBUG][expander] input: %s\n", input);  

    if (strcmp(input, "<END>") == 0)
    return strdup("<END>");

    // allocate output string (worst case: each char doubled + null terminator)
    size_t len = strlen(input);
    char* result = malloc(len * 2 + 1);
    if (!result) return NULL;

    // expand each character
    int j = 0;
    for (int i = 0; input[i]; ++i) {
        result[j++] = input[i];
        result[j++] = input[i];
    }
    result[j] = '\0';

    printf("[PLUGIN][expander] transformed: %s\n", result);
    return result;
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "expander", queue_size);
}