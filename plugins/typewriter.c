#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "plugin_common.h"

// returns per-char delay in ms, 0 means no delay 
static unsigned tw_delay_ms(void) {
    unsigned delay_ms = 100; /* default 100ms */
    const char* env = getenv("FAST_TYPEWRITER");
    if (env && *env) {
        char* endp = NULL;
        unsigned long v = strtoul(env, &endp, 10);
        delay_ms = (endp && *endp == '\0') ? (unsigned)v : 0;
    }
    return delay_ms;
}

static const char* tw_transform(const char* text) {
    if (!text) return NULL;
    
    if (strcmp(text, "<END>") == 0) {
        return strdup("<END>");
        } 

    unsigned delay_ms = tw_delay_ms();

    // print as one unit to avoid interleaving with other threads 
    flockfile(stdout);
    printf("[typewriter] ");
    for (size_t i = 0; text[i] != '\0'; ++i) {
        putchar((unsigned char)text[i]);
        if (delay_ms > 0) usleep(delay_ms * 1000);
    }
    putchar('\n');
    fflush(stdout);
    funlockfile(stdout);

    // forward unchanged 
    return strdup(text);
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(tw_transform, "typewriter", queue_size);
}