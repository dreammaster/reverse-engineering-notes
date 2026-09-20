/* ags/gamesetup.h -- the static game-design data loaded once at
 * startup from ac2game.dta: MouseCursor, InterfaceElement,
 * InventoryItemInfo, and the big one, GameSetupStructBase. Direct
 * port of apply_structs.py's SAFE_DECLS.
 *
 * GameSetupStructBase's real identity is `OriGameSetupStruct`
 * (Common/acroom.h:2769) -- AGS's own OLDEST ancestor struct in its
 * save-compatibility evolution chain, preserved read-only in the 2011
 * header purely for old-save upgrading. FULLY MAPPED: every byte from
 * +0x00 through its confirmed 0xBF84 total is accounted for -- the
 * largest and longest-running single-struct effort in this whole
 * reversing project (see reversing/notes/struct-layout-drift.md for
 * the complete round-by-round history).
 */
#ifndef AGS_GAMESETUP_H
#define AGS_GAMESETUP_H

#include "ags/types.h"
#include "ags/room.h" /* struct EventBlock */

struct CharacterInfo; /* ags/character.h -- forward ref only, chars is an array pointer */
struct WordsDictionary; /* ags/dialog.h -- forward ref only */
struct ccScript; /* ags/script.h -- forward ref only */

/* MouseCursor -- game.mcurs[]'s element type. A rare zero-drift match
 * to 2011's own current declaration (Common/acroom.h). */
struct MouseCursor {
    int pic;             /* +0x00 */
    short hotx;            /* +0x04 */
    short hoty;             /* +0x06 */
    short view;              /* +0x08, -1 = static cursor, no idle animation */
    char name[10];            /* +0x0A, MEDIUM confidence: no direct access site found */
    char flags;                /* +0x14 */
    char _pad_align[3];          /* +0x15..0x18, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct MouseCursor) == 0x18, "MouseCursor must be 0x18 bytes");

/* InterfaceElement -- game.iface[]'s element type, this build's own
 * legacy icon-bar interface system. Confirmed via its own default
 * constructor (InterfaceElement::InterfaceElement); the ENGINE never
 * reads most of these fields at runtime (a fully separate question
 * from the DATA existing -- see reversing/scripts/
 * dump_interface_elements.py, which decoded a real, coherent icon-bar
 * UI design in Rob Blanc 1's own shipped data despite that). `button`
 * is a raw byte blob (InterfaceButton[MAXBUTTON=20], never itself
 * independently confirmed field-by-field). */
struct InterfaceElement {
    int x, y, x2, y2;         /* +0x00..0x10, real by DATA, not independently confirmed by code */
    int bgcol;                  /* +0x10 */
    int fgcol;                    /* +0x14 */
    int bordercol;                  /* +0x18 */
    int vtextxp;                      /* +0x1C */
    int vtextyp;                        /* +0x20 */
    int vtextalign;                      /* +0x24, UNCONFIRMED -- source's own constructor doesn't set it */
    char vtext[40];                        /* +0x28 */
    int numbuttons;                          /* +0x50 */
    char button[0x2D0];                        /* +0x54..0x324, InterfaceButton[MAXBUTTON=20], internal layout unconfirmed */
    int flags;                                    /* +0x324 */
    int reserved_for_future;                        /* +0x328, MEDIUM confidence: positional only */
    int popupyp;                                      /* +0x32C, MEDIUM confidence: positional only */
    char popup;                                         /* +0x330 */
    char on;                                              /* +0x331 */
    char _pad_align[2];                                    /* +0x332..0x334, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct InterfaceElement) == 0x334, "InterfaceElement must be 0x334 bytes");

/* InventoryItemInfo -- game.invinfo[]'s element type. `cursorPic` is
 * CONFIRMED ABSENT (2011's own source documents it as a later
 * addition kept in sync with `pic` purely for save-compat with builds
 * from exactly this era) -- `pic` alone serves both roles here. */
struct InventoryItemInfo {
    char name[25];             /* +0x00 */
    char _pad_align1[3];         /* +0x19..0x1C, compiler alignment padding */
    int pic;                       /* +0x1C */
    char _unconfirmed2[4];           /* +0x20..0x24, CONFIRMED ABSENT: 2011's cursorPic */
    int hotx;                          /* +0x24 */
    int hoty;                            /* +0x28 */
    char _unconfirmed3[0x14];              /* +0x2C..0x40, matches 2011's int reserved[5] */
    char flags;                              /* +0x40 */
    char _pad_align4[3];                       /* +0x41..0x44, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct InventoryItemInfo) == 0x44, "InventoryItemInfo must be 0x44 bytes");

/* GameSetupStructBase -- gamesetup/thisgame/game, the single static
 * game-design blob loaded once from ac2game.dta. Real identity:
 * `OriGameSetupStruct` (see file-level comment above). */
struct GameSetupStructBase {
    char gamename[30];                                 /* +0x00, e.g. "Rob Blanc I" verbatim */
    unsigned char options[20];                            /* +0x1E, OPT_* -- 19/20 indices individually confirmed */
    unsigned char paluses[256];                              /* +0x32 */
    int defpal[256];                                           /* +0x132..0x532 */
    char _pad_align2[2];                                         /* +0x532..0x534, compiler alignment padding */
    struct InterfaceElement iface[10];                             /* +0x534..0x253C */
    int numiface;                                                    /* +0x253C */
    int numviews;                                                      /* +0x2540 */
    struct MouseCursor mcurs[10];                                        /* +0x2544..0x2634 */
    char *globalscript;                                                    /* +0x2634, medium-high confidence */
    int numcharacters;                                                       /* +0x2638 */
    struct CharacterInfo *chars;                                               /* +0x263C */
    struct EventBlock __charcond[50];                                            /* +0x2640..0x4328 */
    struct EventBlock __invcond[100];                                              /* +0x4328..0x7CF8 */
    struct ccScript *compiled_script;                                                /* +0x7CF8 */
    int playercharacter;                                                               /* +0x7CFC */
    unsigned char _pad_unknown3[0x834];                                                  /* +0x7D00..0x8534, unidentified skip (2100 bytes) -- see dump_characters_from_data.py */
    int totalscore;                                                                        /* +0x8534 */
    short numinvitems;                                                                       /* +0x8538 */
    char _pad_align5[2];                                                                       /* +0x853A..0x853C, compiler alignment padding */
    struct InventoryItemInfo invinfo[100];                                                       /* +0x853C..0x9FCC */
    int numdialog;                                                                                 /* +0x9FCC */
    int numdlgmessage;                                                                               /* +0x9FD0 */
    int numfonts;                                                                                      /* +0x9FD4 */
    int color_depth;                                                                                     /* +0x9FD8 */
    int target_win;                                                                                        /* +0x9FDC, MEDIUM confidence: positional only */
    int dialog_bullet;                                                                                       /* +0x9FE0 */
    short hotdot;                                                                                              /* +0x9FE4 */
    short hotdotouter;                                                                                           /* +0x9FE6 */
    int uniqueid;                                                                                                  /* +0x9FE8 */
    int reserved[2];                                                                                                /* +0x9FEC..0x9FF4, medium confidence */
    short numlang;                                                                                                    /* +0x9FF4, medium confidence */
    char langcodes[5][3];                                                                                              /* +0x9FF6..0xA005 */
    char _pad_align3[3];                                                                                                 /* +0xA005..0xA008, compiler alignment padding */
    void *messages[500];                                                                                                    /* +0xA008..0xA7D8 */
    unsigned char fontflags[10];                                                                                              /* +0xA7D8..0xA7E2, medium confidence */
    char fontoutline[10];                                                                                                        /* +0xA7E2..0xA7EC, medium confidence */
    int numgui;                                                                                                                     /* +0xA7EC */
    struct WordsDictionary *dict;                                                                                                     /* +0xA7F0 */
    int reserved2[8];                                                                                                                    /* +0xA7F4..0xA814 */
    unsigned char spriteflags[6000];                                                                                                        /* +0xA814..0xBF84, MAX_SPRITES=6000 (this build's own reduced limit) */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GameSetupStructBase) == 0xBF84, "GameSetupStructBase must be 0xBF84 bytes");

#endif /* AGS_GAMESETUP_H */
