/* ags/saveload.h -- M11 ("the long tail" / save-restore, see
 * src/PLAN.md): a real, scoped port of SaveGameSlot/restore_game_data
 * (rob_blanc_1.asm, proc bounds SaveGameSlot 0x41E228-0x41E463,
 * restore_game_data 0x41E76B onward -- read directly, ~1500 combined
 * lines, since matches.json's own prose had only ever covered
 * individual pieces of each, never the complete real write/read
 * order end to end).
 *
 * REAL FILE FORMAT HEADER (byte-identical to the original, read
 * directly from SaveGameSlot's own opening instructions):
 *   sprintf(name, "agssave.%03d", slotnum)   -- no directory prefix,
 *       this build's own confirmed omission of a $SAVEGAMEDIR$-style
 *       prefix (same drift already established for FileOpen)
 *   fwrite("Adventure Game Studio saved game", strlen+1, 1, f)
 *   fputstring(description, f)               -- Common/cscommon.cpp
 *   putw(7, f)                                -- this build's own
 *       save-format version marker (a real, confirmed literal, not
 *       2011's later numbering)
 * restore_game_data's own real behavior on read: fread the signature
 * block back and strcmp it (mismatch -> real "not a save file" error,
 * -2 here); fgetstring the description; getw() the version and
 * require ==7 (mismatch -> -3 here); if a non-NULL description
 * buffer was supplied, strcpy the description into it and return
 * immediately WITHOUT touching anything past this point (this is
 * GetSaveSlotDescription's own real code path, not a separate
 * function in this build at all -- see matches.json's own
 * restore_game_data entry for the full "fused function, selected by
 * one argument" finding).
 *
 * SCOPE DECISION past the header (read before extending this file):
 * SaveGameSlot's own real body continues for ~330 more lines after
 * the header, writing (in this exact order): the compiled global
 * script's own live globaldata segment, a sync of the currently-
 * loaded room's own live room-script globaldata back into
 * roomstats[displayed_room].tsdata, the WHOLE roomstats[] array
 * (RoomStatus[MAX_ROOMS=300], 0x1390 bytes each) plus each room's own
 * variable-length tsdata blob, GameState (play, 0x964 bytes -- THIS
 * piece IS ported here, see below), MoveList[60] (mls[], 0x200 bytes
 * each), the WHOLE GameSetupStructBase (0xBF84 bytes -- yes, the
 * entire main game struct gets re-saved every time), CharacterInfo[]
 * (THIS piece IS ported here too), the pending-newroom number, the
 * current 256-entry palette, each DialogTopic's own optionflags[15]
 * (THIS piece IS ported here too -- matching the original's own
 * scoped choice to persist only this ONE mutable per-topic field, not
 * the whole topic), several bare mouse-cursor/pause-state globals,
 * write_gui's own GUIMain[]+six-control-array bulk dump, the current
 * room's walk_area_light[] override array, MIDI/MP3 playback position,
 * the single-channel ambient-sound state block, the ScreenOverlay[]
 * array plus each overlay's own serialized bitmap image.
 *
 * This engine has NOT built persistent global state for MOST of
 * that list yet (roomstats[]/MoveList[60]/ScreenOverlay[]/ambient-
 * sound-wrapper/MIDI-position-restore are each their own not-yet-
 * built subsystem -- every milestone test through M11 loads exactly
 * one room/GUI set ad hoc rather than keeping a persistent "current
 * game" singleton the way the original engine's own globals do), so
 * a byte-compatible save file covering ALL of it isn't achievable
 * without building those subsystems first -- a separate, larger
 * effort than this milestone slice. Instead, ags_save_game_slot/
 * ags_restore_game_data here cover exactly the THREE pieces marked
 * above -- GameState, CharacterInfo[], and DialogTopic.optionflags[]
 * -- using the EXACT SAME on-disk field format/order the original
 * uses for each (so a byte-editor/hex-dump comparison against a real
 * save's own corresponding bytes would match), plus one deliberate,
 * clearly-non-original addition: explicit numcharacters/numdialog/
 * current_room fields, since this engine has no other mechanism yet
 * to validate a save was made against compatible game data or to
 * know which room to reload (the original's own "!Restore_Game: Game
 * has changed" checks -- confirmed present via matches.json's own
 * string evidence -- serve the same real PURPOSE, just via a
 * different, not-yet-traced mechanism). This makes this module a
 * real, working, restorable save/restore for everything THIS engine
 * currently tracks as persistent game state -- not a full save-game
 * compatibility layer with the original engine's every subsystem.
 */
#ifndef AGS_SAVELOAD_H
#define AGS_SAVELOAD_H

#include "ags/gamestate.h"
#include "ags/character.h"
#include "ags/dialog.h"

enum AgsSaveLoadError {
    AGS_SAVE_OK = 0,
    AGS_SAVE_OPEN_ERROR = -1,       /* source's own fopen() failure */
    AGS_SAVE_NOT_A_SAVE_FILE = -2,  /* source's own signature strcmp mismatch */
    AGS_SAVE_BAD_VERSION = -3,      /* source's own "getw()!=7" check */
    AGS_SAVE_COUNT_MISMATCH = -4    /* this module's own addition -- see the file-level comment above */
};

/* SaveGameSlot(int, const char*) -- real header (signature/
 * description/version) then, before any real data, this module's own
 * numcharacters/numdialog/current_room addition (written early and
 * validated first on restore, precisely so a wrong caller-supplied
 * count is caught cleanly instead of misaligning every read after
 * it), then the real scoped body: GameState (play, 0x964 bytes, one
 * fwrite -- matching source's own literal ElementSize exactly),
 * CharacterInfo[numcharacters] (0x140 bytes each), each
 * dialogs[i].optionflags[15] (4 bytes each, matching source's own
 * literal ElementCount=0xF/ElementSize=4). */
enum AgsSaveLoadError ags_save_game_slot(int slotnum, const char *description,
                                          const struct GameState *play,
                                          const struct CharacterInfo *chars, int numcharacters,
                                          const struct DialogTopic *dialogs, int numdialog,
                                          int current_room);

/* GetSaveSlotDescription's own real code path (restore_game_data
 * called with a non-NULL description buffer -- see this header's own
 * file-level comment): reads only the header, no game-state
 * deserialization at all. `out_description` must be at least 200
 * bytes (matching this build's own real Str1 buffer size headroom).
 * Returns 0 (matching source's own "return 1" convention inverted to
 * this project's own enum, i.e. AGS_SAVE_OK) on success. */
enum AgsSaveLoadError ags_get_save_slot_description(int slotnum, char *out_description);

/* restore_game_data(int, NULL) -- the full-restore path. `chars` must
 * already be allocated with exactly `numcharacters` slots and
 * `dialogs` with exactly `numdialog` slots (the SAME counts the save
 * was made with -- see AGS_SAVE_COUNT_MISMATCH above); on success,
 * *play/chars[]/dialogs[].optionflags[]/*out_current_room are all
 * overwritten with the saved values. */
enum AgsSaveLoadError ags_restore_game_data(int slotnum, struct GameState *play,
                                             struct CharacterInfo *chars, int numcharacters,
                                             struct DialogTopic *dialogs, int numdialog,
                                             int *out_current_room);

#endif /* AGS_SAVELOAD_H */
