#include <stdio.h>
#include <unistd.h>    // usleep
#include <string.h>
#include <stdlib.h>
#include "plugin_common.h"

static unsigned get_delay_ms(void) {
    // Default: 100ms per character
    unsigned delay_ms = 100;
    const char* env = getenv("FAST_TYPEWRITER");
    if (env && *env) {
        // If numeric, use that value (in ms). Any non-empty non-numeric => zero delay.
        char *endp = NULL;
        unsigned long v = strtoul(env, &endp, 10);
        if (endp && *endp == '\0') {
            delay_ms = (unsigned)v;
        } else {
            delay_ms = 0;
        }
    }
    return delay_ms;
}

static const char* typewriter_transform(const char* input) {
    if (!input) return NULL;

    // Do not print on <END>; just propagate it downstream
    if (strcmp(input, "<END>") == 0) {
        return strdup("<END>");
    }

    // Print exactly in the expected format
    printf("[typewriter] ");
    fflush(stdout);

    unsigned delay_ms = get_delay_ms();
    for (size_t i = 0; input[i]; ++i) {
        putchar((unsigned char)input[i]);
        fflush(stdout);
        if (delay_ms > 0) {
            usleep((useconds_t)delay_ms * 1000);  // ms -> µs
        }
    }
    putchar('\n');
    fflush(stdout);

    // Forward the exact same text downstream
    return strdup(input);
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(typewriter_transform, "typewriter", queue_size);
}

