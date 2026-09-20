/* ags/view.h -- MoveList, ViewFrame272, ViewStruct272. Direct port of
 * apply_structs.py's SAFE_DECLS.
 */
#ifndef AGS_VIEW_H
#define AGS_VIEW_H

#include "ags/types.h"

/* MoveList -- mls[]'s element type, a Bresenham-line-based walk-path
 * plan. One of this project's cleanest zero-drift matches. `direct`
 * is CONFIRMED ABSENT (checked against all three of 2011's own write
 * sites, none present in this build). */
struct MoveList {
    int pos[40];        /* +0x000..0x0A0, packed (tox<<16)|toy waypoints */
    int numstage;          /* +0x0A0 */
    int xpermove[40];         /* +0x0A4..0x144, fixed-point per-move X delta */
    int ypermove[40];           /* +0x144..0x1E4 */
    int fromx;                    /* +0x1E4 */
    int fromy;                      /* +0x1E8 */
    int onstage;                      /* +0x1EC */
    int onpart;                         /* +0x1F0 */
    int lastx;                            /* +0x1F4 */
    int lasty;                              /* +0x1F8 */
    char doneflag;                            /* +0x1FC */
    char direct;                                /* +0x1FD, CONFIRMED ABSENT in this build */
    char _pad_align[2];                           /* +0x1FE..0x200, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct MoveList) == 0x200, "MoveList must be 0x200 bytes");

/* ViewFrame272 -- one animation frame, embedded inside each
 * ViewStruct272 loop-block. Shared by cursors, objects, and
 * characters alike. xoffs/yoffs/sound/reserved_for_future remain
 * unreached by any already-matched caller (structurally present,
 * behaviorally unconfirmed); `flags` bit 0 IS exercised (a real
 * vertical-flip render pass) but doesn't match 2011's own
 * VFLG_FLIPSPRITE (dated after this build's own era, and horizontal
 * not vertical) -- left deliberately unidentified rather than forced. */
struct ViewFrame272 {
    int pic;                   /* +0x00, -1 = unused slot sentinel */
    short xoffs;                  /* +0x04, UNCONFIRMED */
    short yoffs;                     /* +0x06, UNCONFIRMED */
    short speed;                       /* +0x08 */
    char _pad_align[2];                  /* +0x0A..0x0C, compiler alignment padding */
    int flags;                             /* +0x0C, bit 0 exercised (vertical flip), role otherwise unconfirmed */
    int sound;                                /* +0x10, UNCONFIRMED */
    int reserved_for_future[2];                 /* +0x14..0x1C, UNCONFIRMED */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ViewFrame272) == 0x1C, "ViewFrame272 must be 0x1C bytes");

/* ViewStruct272 -- views[]'s element type. DRIFT: 8 loops x 10 frames
 * (80 slots) here vs. 2011's 16x20(320); 2011's separate
 * loopflags[16] array is CONFIRMED ABSENT -- the header ends
 * immediately after numframes[8] with zero slack. */
struct ViewStruct272 {
    short numloops;                    /* +0x00 */
    short numframes[8];                  /* +0x02..0x12 */
    char _pad_align[2];                    /* +0x12..0x14, compiler alignment padding */
    struct ViewFrame272 frames[8][10];       /* +0x14..0x8D4 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ViewStruct272) == 0x8D4, "ViewStruct272 must be 0x8D4 bytes");

#endif /* AGS_VIEW_H */
