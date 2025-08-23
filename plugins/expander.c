#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

static const char* expander_transform(const char* in) {
    if (!in) return NULL;
    if (strcmp(in, "<END>") == 0) return strdup("<END>");

    size_t n = strlen(in);
    if (n == 0) return strdup("");          // קלט ריק → מחרוזת ריקה

    // פלט באורך 2*n - 1: תו, רווח, תו, רווח, ... בלי רווח בסוף
    size_t out_len = (n > 0) ? (2*n - 1) : 0;
    char* out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t p = 0;
    for (size_t i = 0; i < n; ++i) {
        out[p++] = in[i];
        if (i + 1 < n) out[p++] = ' ';
    }
    out[p] = '\0';
    return out;
}

const char* plugin_init(int qsz) {
    return common_plugin_init(expander_transform, "expander", qsz);
}



/* #include <stdio.h>
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
} */