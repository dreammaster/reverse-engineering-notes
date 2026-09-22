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
        /* `func` must be part of the identity too, not just (file,
         * line): SCMD_CALLEXT (ags/interp.c) deliberately passes the
         * SAME "<script CALLEXT>"/0 pseudo-location for every native
         * call regardless of which function was actually named --
         * without this check, the SECOND distinct native function
         * ever logged from a script would silently be counted under
         * the FIRST one's name instead of getting its own entry (a
         * real bug this project's own M11 native-API dispatch round
         * found: comparing two runs' hit counts for the same name
         * disagreed, tracing back to exactly this). strcmp, not
         * pointer equality -- a resolved import name is the same
         * pointer across repeat calls to the SAME import, but not
         * guaranteed across different ones. */
        if (s_sites[i].line == line && s_sites[i].file == file &&
            strcmp(s_sites[i].func, func) == 0) {
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
