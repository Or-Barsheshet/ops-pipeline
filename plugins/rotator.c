#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

static const char* rotate_right_once(const char* input_str) {
    if (!input_str) return NULL;

    if (strcmp(input_str, "<END>") == 0) {
        return strdup("<END>"); 
        }

    size_t len = strlen(input_str);
    if (len == 0) return strdup("");

    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;

    // last char goes to front, others shift right by one 
    out[0] = input_str[len - 1];
    memcpy(out + 1, input_str, len - 1);
    out[len] = '\0';
    return out;
}

const char* plugin_init(int qsz) {
    return common_plugin_init(rotate_right_once, "rotator", qsz);
}