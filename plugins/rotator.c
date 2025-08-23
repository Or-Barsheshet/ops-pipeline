#include <stdlib.h>
#include <string.h>
#include "plugin_common.h"

static const char* rotator_transform(const char* in) {
    if (!in) return NULL;
    if (strcmp(in, "<END>") == 0) return strdup("<END>");

    size_t n = strlen(in);
    if (n == 0) return strdup("");

    char* out = malloc(n + 1);
    if (!out) return NULL;

    // סיבוב ימינה: התו האחרון לראש, והשאר מוזזים ימינה
    out[0] = in[n - 1];
    memcpy(out + 1, in, n - 1);
    out[n] = '\0';
    return out;
}

const char* plugin_init(int qsz) {
    return common_plugin_init(rotator_transform, "rotator", qsz);
}
