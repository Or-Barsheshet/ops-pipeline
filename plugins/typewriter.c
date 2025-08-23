#include <stdio.h>
#include <unistd.h>    // usleep
#include <string.h>
#include <stdlib.h>
#include "plugin_common.h"

static unsigned get_delay_ms(void) {
    unsigned delay_ms = 100;                  // default: 100ms/char
    const char* env = getenv("FAST_TYPEWRITER");
    if (env && *env) {
        char *endp = NULL;
        unsigned long v = strtoul(env, &endp, 10);
        delay_ms = (endp && *endp == '\0') ? (unsigned)v : 0;  // numeric -> that ms, else 0
    }
    return delay_ms;
}

static const char* typewriter_transform(const char* input) {
    if (!input) return NULL;
    if (strcmp(input, "<END>") == 0) {
        return strdup("<END>");
    }

    unsigned delay_ms = get_delay_ms();

    // Print the whole line atomically w.r.t. other threads
    flockfile(stdout);

    printf("[typewriter] ");
    for (size_t i = 0; input[i]; ++i) {
        putchar((unsigned char)input[i]);
        if (delay_ms > 0) {
            usleep(delay_ms * 1000);  // ms -> µs
        }
    }
    putchar('\n');
    fflush(stdout);   // single flush per line
    funlockfile(stdout);

    return strdup(input);  // forward unchanged
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(typewriter_transform, "typewriter", queue_size);
}
