/* ags/room.h -- room-format structs. Direct port of apply_structs.py's
 * SAFE_DECLS (EventBlock/AnimationStruct/FullAnimation/sprstruc/
 * PolyPoints/RoomObject/RoomStatus/RoomStruct); see
 * reversing/notes/struct-layout-drift.md for the full derivation of
 * every field and reversing/scripts/apply_structs.py for the byte-
 * identical IDA declarations this header mirrors. See ags/types.h for
 * the natural-alignment assumption these layouts depend on.
 */
#ifndef AGS_ROOM_H
#define AGS_ROOM_H

#include "ags/types.h"

/* Opaque forward reference only -- RoomStruct.compiled_script is a
 * pointer to a ccScript (see ags/script.h), but room.h itself doesn't
 * need that struct's fields. */
struct ccScript;

/* EventBlock -- this build's own live interaction-script format
 * (Common/acroom.h:239-246; still declared, but only as a dead,
 * commented-out historical footnote, in the 2011 reference source --
 * this build's own room-loading/interaction code still actively reads
 * and executes it). Used by GameSetupStructBase (__charcond/
 * __invcond), RoomStatus (hscond/objcond/misccond), and RoomStruct
 * (hscond/objcond/misccond, the SOURCE copies RoomStatus's own fields
 * are runtime copies of). Total confirmed size 0x94 (148 bytes), zero
 * drift from 2011's own declared size.
 */
struct EventBlock {
    int list[8];        /* +0x00, MAXCOMMANDS=8 (Common/acroom.h:238) */
    int respond[8];      /* +0x20, this build's own respond[] enum (15 confirmed values, 0-0xE) */
    int respondval[8];    /* +0x40 */
    int data[8];         /* +0x60 */
    int numcmd;          /* +0x80, loop bound over list/respond/respondval/data */
    short score[8];       /* +0x84 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct EventBlock) == 0x94, "EventBlock must be 0x94 bytes");

/* AnimationStruct -- 2011's own still-declared struct (Common/
 * acroom.h:218-224; formerly this project's project-invented
 * "EventBlockCmd" placeholder, retired once the real 2011 identity was
 * found). One command in a FullAnimation's command list. */
struct AnimationStruct {
    int x;         /* +0x00 (was data0): move/set-position target X */
    int y;         /* +0x04 (was data1): move/set-position target Y */
    int data;      /* +0x08 (was data2): view number, sprite number, etc. depending on action */
    int object;    /* +0x0C (was target): the character/object index this command targets */
    int speed;     /* +0x10 (was data3) */
    char action;   /* +0x14 (was type): the command opcode, 0-5 confirmed exhaustive */
    char wait;     /* +0x15 (was waitUntilDone) */
    char _pad_align[2]; /* +0x16..0x18, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct AnimationStruct) == 0x18, "AnimationStruct must be 0x18 bytes");

/* FullAnimation -- 2011's own still-declared struct (Common/
 * acroom.h:226-232; formerly this project's placeholder "GameAnimation"
 * for its OTHER role, the entirely-undocumented-in-2011 10-slot global
 * "Animations" resource table this build indexes via
 * EventBlock.respond[i]==4). Used both as RoomStruct.anims[10] (the
 * on-disk format 2011 keeps declared but no longer reads) and as that
 * separate global resource table. */
struct FullAnimation {
    struct AnimationStruct stage[10]; /* +0x000..0xF0 (was command[10]) */
    int numstages;                    /* +0xF0 (was numCommands) */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct FullAnimation) == 0xF4, "FullAnimation must be 0xF4 bytes");

/* sprstruc -- RoomStruct.sprs[]'s own element type (2011's
 * `_sprstruc`-equivalent, recovered via its own default constructor).
 * No padding needed: an all-short struct, size already a multiple of
 * the struct's own 2-byte alignment requirement. */
struct sprstruc {
    short sprnum; /* +0x00 */
    short x;      /* +0x02 */
    short y;      /* +0x04 */
    short room;   /* +0x06 */
    short on;     /* +0x08, confirmed via sprstruc::sprstruc()'s own constructor */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct sprstruc) == 0x0A, "sprstruc must be 0x0A bytes");

/* PolyPoints -- RoomStruct.wallpoints[]'s own element type
 * (Common/acroom.h:252-255), used only by the AGS Editor in 2011 (this
 * build's own engine never reads numpoints -- editor-only data still
 * loaded at runtime). */
struct PolyPoints {
    int x[30];      /* +0x000..0x78 */
    int y[30];      /* +0x078..0xF0 */
    int numpoints;  /* +0xF0, confirmed via PolyPoints::PolyPoints()'s own constructor */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct PolyPoints) == 0xF4, "PolyPoints must be 0xF4 bytes");

/* RoomObject -- croom->obj[]'s own element type (2011's `RoomObject`,
 * far smaller here: no tint/zoom/last-width/last-height/blocking-box
 * fields at all -- all CONFIRMED ABSENT, later AGS additions this 2002
 * build predates). Every field HIGH confidence; this struct has no
 * open fields left in this project's own investigation. */
struct RoomObject {
    int x;               /* +0x00 */
    int y;                /* +0x04 */
    int transparent;      /* +0x08 */
    short num;             /* +0x0C, the sprite slot number */
    short baseline;        /* +0x0E */
    short view;            /* +0x10, -1 = not view-animated */
    short loop;            /* +0x12 */
    short frame;           /* +0x14 */
    short wait;            /* +0x16 */
    short moving;          /* +0x18, MoveList slot index + 1, or 0 = not moving */
    char cycling;          /* +0x1A */
    char overall_speed;    /* +0x1B */
    char on;               /* +0x1C, 0/1 normal, 2 = merged (MergeObject) */
    char flags;            /* +0x1D, bit0=OBJF_NOINTERACT, bit1=OBJF_NOWALKBEHINDS */
    char _pad_align[2];    /* +0x1E..0x20, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct RoomObject) == 0x20, "RoomObject must be 0x20 bytes");

/* RoomStatus -- roomstats[roomnumber], the per-save-slot runtime state
 * for a room the player has visited at least once. FULLY MAPPED --
 * every byte from +0x00 to its confirmed 0x1390 total is either a
 * confirmed field or an explicitly-evidenced pad, no open territory. */
struct RoomStatus {
    int beenhere;                    /* +0x00 */
    int numobj;                       /* +0x04 */
    struct RoomObject obj[10];        /* +0x08..0x148 (DRIFT: 10 here vs. 2011's MAX_INIT_SPR=40) */
    short flagstates[15];             /* +0x148..0x166 (MAX_FLAGS=15, zero drift) */
    char _pad_align[2];               /* +0x166..0x168, compiler alignment padding */
    int tsdatasize;                   /* +0x168 */
    char *tsdata;                      /* +0x16C */
    struct EventBlock hscond[20];      /* +0x170..0xD00 (MAX_HOTSPOTS=20, this build's original value) */
    struct EventBlock objcond[10];     /* +0xD00..0x12C8 */
    struct EventBlock misccond;         /* +0x12C8..0x135C */
    char hotspot_enabled[20];          /* +0x135C..0x1370 */
    short walkbehind_base[15];         /* +0x1370..0x138E (MAX_WALK_AREAS=15) */
    char _pad_align6[2];               /* +0x138E..0x1390, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct RoomStatus) == 0x1390, "RoomStatus must be 0x1390 bytes");

/* RoomStruct -- thisroom/rstruc, the currently-loaded room's own
 * static (on-disk) data. Its own total size has no hard sizeof/
 * malloc-literal anchor (struct-layout-drift.md), so no
 * AGS_STATIC_ASSERT on total size here -- +0x3A20 (where ebscene[]
 * ends) is the last byte any reachable code path in this build ever
 * touches, treated as this struct's practical size, not a proven one.
 * `block`/`ccScript*` fields per ags/types.h and the ccScript forward
 * declaration above. */
struct RoomStruct {
    char _pad_unknown_0[4];               /* +0x00, role NOT confirmed -- see struct-layout-drift.md */
    block walls;                           /* +0x04, walkable-area mask bitmap */
    block object;                          /* +0x08, walk-behind mask bitmap */
    block lookat;                          /* +0x0C, hotspot mask bitmap */
    block regions;                         /* +0x10, region mask bitmap (loaded, never read by this build) */
    unsigned char pal[256][4];             /* +0x14..0x414, room palette (raw RGB-ish bytes, exact 4th-byte role unconfirmed) */
    short numobj;                           /* +0x414 */
    short objyval[15];                      /* +0x416..0x434 (MAX_OBJ=15 in this build's default constructor) */
    short whataction[130];                  /* +0x434..0x538, v7/v8-only "obsolete v2.00 action editor" data, dead code for Rob Blanc 1's own room files */
    short val1[130];                        /* +0x538..0x63C */
    short val2[130];                        /* +0x63C..0x740 */
    short otcond[130];                      /* +0x740..0x844 */
    char points[130];                       /* +0x844..0x8C6 */
    short left;                             /* +0x8C6, room-edge X (left) */
    short right;                            /* +0x8C8 */
    short top;                              /* +0x8CA */
    short bottom;                           /* +0x8CC */
    short numsprs;                          /* +0x8CE */
    short nummes;                           /* +0x8D0 */
    struct sprstruc sprs[10];                /* +0x8D2..0x936 */
    char password[11];                       /* +0x936..0x941 */
    char options[10];                        /* +0x941..0x94B, this build's ST_* room-level options, all 5 confirmed */
    char _pad_align_message[1];              /* +0x94B..0x94C, compiler alignment padding */
    char *message[100];                       /* +0x94C..0xADC, decrypted, individually malloc'd strings */
    char msgi[100][2];                        /* +0xADC..0xBA4, packed MessageInfo entries (internal layout unconfirmed) */
    short wasversion;                          /* +0xBA4 */
    short flagstates[15];                      /* +0xBA6..0xBC4, MEDIUM confidence (positional) */
    struct FullAnimation anims[10];             /* +0xBC4..0x154C, declared-but-dead-by-2011 on-disk format */
    short numanims;                             /* +0x154C */
    short shadinginfo[16];                       /* +0x154E..0x156E */
    char _pad_align_walkareas[2];                /* +0x156E..0x1570, compiler alignment padding */
    int numwalkareas;                            /* +0x1570 */
    struct PolyPoints wallpoints[15];             /* +0x1574..0x23C0, MAX_WALK_AREAS=15, editor-only data */
    int numhotspots;                              /* +0x23C0 */
    short hswalkto[20][2];                         /* +0x23C4..0x2414 */
    char hotspotnames[20][30];                     /* +0x2414..0x266C, DRIFT: fixed inline array here, not 2011's malloc'd char*[] */
    struct EventBlock hscond[20];                   /* +0x266C..0x31FC, SOURCE copy of RoomStatus's own runtime copy */
    struct EventBlock objcond[10];                   /* +0x31FC..0x37C4 */
    struct EventBlock misccond;                       /* +0x37C4..0x3858 */
    int objbaseline[10];                              /* +0x3858..0x3880 */
    short width;                                       /* +0x3880 */
    short height;                                      /* +0x3882 */
    short resolution;                                   /* +0x3884 */
    short walk_area_zoom[16];                            /* +0x3886..0x38A6 */
    short walk_area_light[16];                            /* +0x38A6..0x38C6 */
    char objectnames[10][30];                              /* +0x38C6..0x39F2 */
    char _pad_align_scripts[2];                             /* +0x39F2..0x39F4, compiler alignment padding */
    char *scripts;                                           /* +0x39F4, malloc'd raw text-script source, freed via plain free() */
    struct ccScript *compiled_script;                         /* +0x39F8, freed via ccFreeScript() */
    int cscriptsize;                                          /* +0x39FC */
    int num_bscenes;                                          /* +0x3A00, default 1 */
    int bscene_anim_speed;                                    /* +0x3A04, default 5 */
    int bytes_per_pixel;                                       /* +0x3A08 */
    block ebscene[5];                                          /* +0x3A0C..0x3A20, MAX_BSCENE background-frame bitmaps (capacity not independently confirmed against the disassembly, taken from 2011's own MAX_BSCENE=5) */
} AGS_PACKED_STRUCT;

#endif /* AGS_ROOM_H */
