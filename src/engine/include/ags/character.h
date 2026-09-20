/* ags/character.h -- CharacterInfo. Direct port of apply_structs.py's
 * SAFE_DECLS. Matches 2011's save-compat ancestor `OldCharacterInfo`
 * (Common/acroom.h:2599) almost field-for-field, with the `talkcolor`
 * byte packed into the top byte of `flags` rather than being its own
 * field (see struct-layout-drift.md's `SetTalkingColor` round) -- one
 * of the most thoroughly, independently confirmed structs in this
 * project; no field remains open. See ags/room.h/ags/gamesetup.h for
 * how this struct is referenced (GameSetupStructBase.chars, an array).
 */
#ifndef AGS_CHARACTER_H
#define AGS_CHARACTER_H

#include "ags/types.h"

struct CharacterInfo {
    int defview;              /* +0x00, the view number ChangeCharacterView reverts to */
    int talkview;              /* +0x04 */
    int view;                   /* +0x08, current view, -1 = not view-animated */
    int room;                    /* +0x0C */
    int prevroom;                 /* +0x10 */
    int x;                         /* +0x14 */
    int y;                          /* +0x18 */
    int wait;                        /* +0x1C, also this build's walk-frame timer (animwait/walkwait folded in) */
    int flags;                        /* +0x20, CHF_* bits; top byte doubles as the packed talkcolor (OCHF_SPEECHCOL) */
    short following;                   /* +0x24, character index being followed, or -1 */
    short followinfo;                   /* +0x26 */
    int idleview;                        /* +0x28 */
    short idletime;                       /* +0x2C */
    short idleleft;                        /* +0x2E */
    short transparency;                    /* +0x30 */
    short baseline;                         /* +0x32 */
    int activeinv;                            /* +0x34, -1 = none active */
    short loop;                                /* +0x38 */
    short frame;                                /* +0x3A */
    short walking;                               /* +0x3C, MoveList slot index + 1, or 0 */
    short animating;                              /* +0x3E, bit1=CHANIM_REPEAT */
    short walkspeed;                               /* +0x40 */
    short animspeed;                                /* +0x42 */
    short inv[100];                                  /* +0x44..0x10C, per-item owned count */
    short actx;                                       /* +0x10C, screen-space render-time cache */
    short acty;                                        /* +0x10E */
    char name[30];                                      /* +0x110..0x12E */
    char scrname[16];                                    /* +0x12E..0x13E, script name (e.g. "EGO") */
    char on;                                              /* +0x13E */
    char _pad_align[1];                                    /* +0x13F..0x140, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct CharacterInfo) == 0x140, "CharacterInfo must be 0x140 bytes");

#endif /* AGS_CHARACTER_H */
