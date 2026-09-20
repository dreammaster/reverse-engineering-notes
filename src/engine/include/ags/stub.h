/* ags/stub.h -- the not-yet-implemented-function convention.
 * See src/PLAN.md ("The stub convention"). A not-yet-implemented
 * function's ENTIRE body is one call to AGS_STUB()/AGS_STUB_VOID() --
 * never a partial implementation, so every commit stays compilable and
 * linkable while the engine is filled in one milestone at a time.
 */
#ifndef AGS_STUB_H
#define AGS_STUB_H

/* Logs (stderr + stub_hits.log) the first time a given call site is
 * hit, deduped by (file, line) so driving the real game through a
 * loop doesn't spam the log -- repeat hits are silently counted
 * instead. Call ags_stub_dump_summary() at shutdown (or on demand) to
 * print every distinct stub hit and its count, in first-hit order:
 * that list IS the data-driven "implement these next" backlog. */
void ags_stub_hit(const char *func, const char *file, int line);
void ags_stub_dump_summary(void);

#define AGS_STUB_VOID() \
    do { ags_stub_hit(__func__, __FILE__, __LINE__); } while (0)

#define AGS_STUB(retval) \
    do { ags_stub_hit(__func__, __FILE__, __LINE__); return (retval); } while (0)

#endif /* AGS_STUB_H */
