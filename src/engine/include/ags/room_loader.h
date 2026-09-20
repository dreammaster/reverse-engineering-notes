/* ags/room_loader.h -- M5 ("Open a room", see src/PLAN.md): a real
 * port of load_room/load_main_block's block-type dispatch (Common/
 * acroom.h:1977/1605, both still present in this repo, cross-checked
 * against reversing/analysis/matches.json's own entries for both AND
 * against rob_blanc_1.asm's real disassembly directly -- this format
 * has genuine, substantial drift from the 2011 reference beyond what
 * matches.json's own prose fully spells out in read order, so this
 * port was built by reading load_room/load_main_block's own compiled
 * bodies end to end, not solely from the 2011 source).
 *
 * SCOPE, this milestone (see ags_load_room's own file-level comment
 * in room_loader.c for the full per-field breakdown):
 *   - Real LZW background decompression (ags/lzw.h's
 *     ags_lzw_expand_to_mem, this module's own load_room_lzw) and
 *     real RLE mask decompression (ags/lzw.h's ags_cunpackbitl, this
 *     module's own load_room_mask) -- both produce real Allegro
 *     BITMAP*s (already linked since M0), per PLAN.md's own "for
 *     real" instruction for this milestone.
 *   - EventBlock command lists (hscond/objcond/misccond) are LOADED
 *     into RoomStruct exactly as the room file stores them, but not
 *     yet DISPATCHED (running them is run_event_block's own job, a
 *     later milestone once script-callable native functions exist to
 *     dispatch INTO).
 *   - Room-format version <9 (whataction/val1/val2/otcond/points, the
 *     "obsolete v2.00 action editor" arrays, auto-converted at load
 *     time into hscond/objcond/misccond) is a CONFIRMED DEAD CODE path
 *     for this specific game -- reversing/scripts/
 *     parse_clib_manifest.py's own --versions output already proved
 *     every one of Rob Blanc 1's real .crm files is version 11 (DOS)
 *     or 13 (Windows), both >=9 -- so it's treated as an explicit
 *     "unsupported" error rather than ported, the same scoping
 *     decision this project's own reversing notes already made for
 *     this exact branch.
 *   - The graphical-scripts sub-block (load_script_configuration +
 *     load_graphical_scripts, Engine/scrptrt.cpp -- a genuinely real
 *     read for any wasversion>=4 room, which Rob Blanc 1's rooms all
 *     are) has its BYTES consumed correctly (keeping the file
 *     position aligned for what follows) but its payload is skipped
 *     via fseek rather than written out to "~acscN.tmp" temp files --
 *     avoiding filesystem side effects from a room-load test/tool,
 *     and the graph-script/"Animations" resource subsystem itself
 *     isn't reachable from anywhere yet in this reimplementation.
 */
#ifndef AGS_ROOM_LOADER_H
#define AGS_ROOM_LOADER_H

#include "ags/room.h"

enum AgsRoomLoadError {
    AGS_ROOM_LOAD_OK = 0,
    AGS_ROOM_LOAD_OPEN_ERROR = -1,
    AGS_ROOM_LOAD_READ_ERROR = -2,
    AGS_ROOM_LOAD_BAD_VERSION = -3,          /* wasversion outside [2,14] -- "Load_Room: Bad packed file..." */
    AGS_ROOM_LOAD_UNSUPPORTED_VERSION = -4,  /* wasversion<9 (or <6) -- see the file-level comment above */
    AGS_ROOM_LOAD_OLD_FORMAT = -5,           /* BLOCKTYPE_COMPSCRIPT/COMPSCRIPT2 (3/4) -- "Load_room: old room format" */
    AGS_ROOM_LOAD_UNKNOWN_BLOCK = -6,        /* "LoadRoom: unknown block type %d encountered" */
    AGS_ROOM_LOAD_INCONSISTENT_OBJECTNAMES = -7, /* "Load_room: inconsistent blocks for object names" */
    AGS_ROOM_LOAD_TOO_MANY_WALKAREAS = -8,   /* "load_room: Too many walkable areas, need newer version" */
    AGS_ROOM_LOAD_OUT_OF_MEMORY = -9,
    AGS_ROOM_LOAD_BAD_SCRIPT = -10,          /* the embedded room script failed its own format checks */
    AGS_ROOM_LOAD_CAPACITY_EXCEEDED = -11    /* a dynamic on-disk count (numobj/numsprs/numwalkareas/nummes) exceeded this build's own fixed array capacity -- see room_loader.c's own note on this being a safety check the original disassembly doesn't have */
};

/* Loads a room by CLIB filename (e.g. "room1.crm") into *rst. `*rst`
 * must be zero-initialized before the very first call (matching the
 * real engine's own fresh `roomstruct rstruc;` global) -- this
 * function's own pre-load cleanup (freeing message[]/scripts/
 * compiled_script, destroying stale ebscene[1..]) only handles what a
 * PREVIOUSLY LOADED room left behind, not first-time garbage. Opens
 * the file via ags_clib_fopen (M1). Returns AGS_ROOM_LOAD_OK (0) on
 * success or a negative AgsRoomLoadError. */
enum AgsRoomLoadError ags_load_room(const char *filename, struct RoomStruct *rst);

#endif /* AGS_ROOM_LOADER_H */
