#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

// insert a single space between characters (no trailing space)
static const char* expand_with_spaces(const char* input_str) {
    if (!input_str) return NULL;
    
    if (strcmp(input_str, "<END>") == 0){
        return strdup("<END>");
    }

    size_t len = strlen(input_str);
    if (len == 0) return strdup("");

    // output length: len chars + (len-1) spaces + '\0'
    size_t out_len = len + (len - 1);
    char* out = (char*)malloc(out_len + 1);
    if (!out) return NULL;

    size_t pos = 0;
    for (size_t i = 0; i < len; ++i) {
        out[pos++] = input_str[i];
        if (i + 1 < len) out[pos++] = ' ';
    }
    out[pos] = '\0';
    return out;
}

const char* plugin_init(int qsz) {
    return common_plugin_init(expand_with_spaces, "expander", qsz);
}