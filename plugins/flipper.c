#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

// reverse the input string
static const char* flip_copy(const char* input_str) {
    if (!input_str) return NULL;
    
    if (strcmp(input_str, "<END>") == 0){
        return strdup("<END>");
    }

    size_t len = strlen(input_str);
    if (len <= 1) return strdup(input_str);

    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;

    for (size_t i = 0; i < len; ++i) {
        out[i] = input_str[len - 1 - i];
    }
    out[len] = '\0';
    return out;
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(flip_copy, "flipper", queue_size);
}