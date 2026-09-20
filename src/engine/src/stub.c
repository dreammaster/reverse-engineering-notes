/* ags/stub.h's own implementation. See src/PLAN.md's "stub convention"
 * section. Deliberately simple (linear scan, fixed-size table) -- this
 * is a development aid, not shipped-game code, and the hit count is
 * expected to stay small (one entry per not-yet-implemented function,
 * not per call).
 */
#include "ags/stub.h"

#include <stdio.h>
#include <string.h>

#define AGS_STUB_MAX_SITES 4096

typedef struct {
    const char *func;
    const char *file;
    int line;
    unsigned long hits;
} AgsStubSite;

static AgsStubSite s_sites[AGS_STUB_MAX_SITES];
static int s_site_count = 0;
static FILE *s_log = NULL;

void ags_stub_hit(const char *func, const char *file, int line)
{
    int i;

    if (!s_log) {
        s_log = fopen("stub_hits.log", "a");
    }

    for (i = 0; i < s_site_count; i++) {
        if (s_sites[i].line == line && s_sites[i].file == file) {
            s_sites[i].hits++;
            return;
        }
    }

    /* First time this exact call site has been hit -- log it. */
    fprintf(stderr, "[AGS_STUB] %s (%s:%d) not implemented yet\n", func, file, line);
    if (s_log) {
        fprintf(s_log, "%s\t%s:%d\n", func, file, line);
        fflush(s_log);
    }

    if (s_site_count < AGS_STUB_MAX_SITES) {
        s_sites[s_site_count].func = func;
        s_sites[s_site_count].file = file;
        s_sites[s_site_count].line = line;
        s_sites[s_site_count].hits = 1;
        s_site_count++;
    }
}

void ags_stub_dump_summary(void)
{
    int i;

    fprintf(stderr, "\n=== AGS_STUB summary: %d distinct not-yet-implemented call site(s) ===\n",
            s_site_count);
    for (i = 0; i < s_site_count; i++) {
        fprintf(stderr, "  %6lu hit(s)  %-40s %s:%d\n",
                s_sites[i].hits, s_sites[i].func, s_sites[i].file, s_sites[i].line);
    }
}
