/* ags/misc.h -- TreeMap (translation lookup), EventHappened (the
 * queued-event ring buffer), and OnScreenWindow (CSCI legacy dialog
 * windows). Direct port of apply_structs.py's SAFE_DECLS.
 */
#ifndef AGS_MISC_H
#define AGS_MISC_H

#include "ags/types.h"

/* TreeMap -- a simple binary search tree mapping original text to its
 * translation (used by init_translation/find_word_in_dictionary-
 * adjacent code). Zero drift from 2011's own 4-pointer declaration;
 * confirmed via a hard allocation-site anchor (TreeMap::addText's own
 * `operator new(0x10)` for a fresh child node). */
struct TreeMap {
    struct TreeMap *left;    /* +0x00 */
    struct TreeMap *right;     /* +0x04 */
    char *text;                  /* +0x08 */
    char *translation;             /* +0x0C */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct TreeMap) == 0x10, "TreeMap must be 0x10 bytes");

/* EventHappened -- the queued-event ring buffer setevent()/
 * process_event() drive (this build's own EVB_HOTSPOT/EVB_ROOM/
 * EV_TEXTSCRIPT/etc. dispatch). Capacity MAXEVENTS=15, genuinely zero
 * drift from 2011 -- unusual for this project. */
struct EventHappened {
    int type;      /* +0x00, EV_TEXTSCRIPT/EV_RUNEVBLOCK/EV_FADEIN/EV_IFACECLICK/EV_NEWROOM */
    int data1;        /* +0x04 */
    int data2;          /* +0x08 */
    int data3;             /* +0x0C */
    int player;               /* +0x10 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct EventHappened) == 0x14, "EventHappened must be 0x14 bytes");

/* OnScreenWindow -- the CSCI legacy dialog subsystem's own window
 * stack (a simple LIFO). Only needed for save/load/setup dialogs --
 * see src/PLAN.md's "explicitly deferred" list; declared here for
 * completeness, not needed until that later milestone. A rare struct
 * that survived completely unchanged from 2002 to 2011. */
struct OnScreenWindow {
    void *buffer;   /* +0x00, saved-background bitmap */
    int x;             /* +0x04 */
    int y;                /* +0x08 */
    int oldtop;             /* +0x0C */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct OnScreenWindow) == 0x10, "OnScreenWindow must be 0x10 bytes");

#endif /* AGS_MISC_H */
