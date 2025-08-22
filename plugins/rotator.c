#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"
#include <stdio.h>

const char* plugin_transform(const char* input) {

    if (!input) return NULL;

    printf("[DEBUG][rotator] input: %s\n", input);  

    if (strcmp(input, "<END>") == 0)
    return strdup("<END>");

    char* result = strdup(input);
    if (!result) return NULL;

    for (int i = 0; result[i]; ++i) {
        char c = result[i];
        if ('a' <= c && c <= 'z') {
            result[i] = ((c - 'a' + 13) % 26) + 'a';
        } else if ('A' <= c && c <= 'Z') {
            result[i] = ((c - 'A' + 13) % 26) + 'A';
        }
    }

    printf("[PLUGIN][rotator] transformed: %s\n", result);
    return result;
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(plugin_transform, "rotator", queue_size);
}