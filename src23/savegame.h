#ifndef YENDOR23_SAVEGAME_H
#define YENDOR23_SAVEGAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * CURGAME / SAVGAMEn: the live game file and its six save slots. A save is
 * a byte-for-byte copy of CURGAME (SaveCurrentGameToSlot / RunGameDialog's
 * load path, yendor2.asm:27246 and :26081), so one format covers both.
 *
 * The file is 7 contiguous sections. Sections 1 and 7 are snapshots of
 * in-memory state written at save time (DS:0x93FF and DS:0x0F26 in
 * Chapter 2); sections 2-6 live only in the file and the game reads and
 * writes them on demand. Section sizes differ between the games (the
 * Chapter 3 world is larger) but every offset is the previous offset plus
 * the previous size, and record sizes match.
 *
 * SaveGame keeps the whole file as one flat image so a load/store round
 * trip is byte-exact, including stale bytes (e.g. the header's name is
 * copied with a plain string copy, so a shorter name leaves the tail of
 * the previous one behind).
 */

typedef enum {
    /* Game-state block (record 0) followed by 9 party records (records 1-9). */
    SaveSectionHeaderAndParty,
    /* Fog-of-war bitmap, one row per record (PersistExploredCell, RevealMapRegion). */
    SaveSectionExploredMap,
    /* Ground-item and container-content item instances. */
    SaveSectionItemInstances,
    /* Byte-addressed persisted state (LoadCurgameRecord, HandleSearchCommand, UnlockDoorCommand). */
    SaveSectionEventState,
    /* Byte-addressed lock/shop state (LoadLockState, RunShopScreen). */
    SaveSectionLockAndShopState,
    /* Bitmap of map cells whose monster has already spawned. */
    SaveSectionMonsterSpawnFlags,
    /* Per-level monster pool (g_levelMonsters). */
    SaveSectionMonsters,
    SaveSectionCount
} SaveSection;

typedef struct {
    uint32_t offset;
    uint32_t size;
    uint16_t recordSize;
    uint16_t recordCount; /* recordSize * recordCount == size */
} SaveSectionInfo;

typedef struct {
    uint32_t totalSize;
    SaveSectionInfo sections[SaveSectionCount];
} SaveLayout;

enum {
    SaveFileSizeYendor2 = 77509,
    SaveFileSizeYendor3 = 81037,
    SaveFileSizeMax = SaveFileSizeYendor3,

    SaveSlotCount = 6,
    SaveSlotFileNameSize = 9, /* "SAVGAME1" + NUL */

    SaveHeaderRecordSize = 500,
    SavePartyRecordSize = 500,
    SavePartyRecordCount = 9,
    SavePartyMemberSlots = 4,
    SaveItemInstanceSize = 34,
    SaveMonsterRecordSize = 156,
    SaveMonsterCount = 80,

    SaveNameMaxLength = 24,
    SaveNameBufferSize = SaveNameMaxLength + 1
};

/*
 * Offsets within section 1's game-state block (record 0). Position, clock,
 * gold and party-slot fields were checked against real Chapter 2 files;
 * gold, ore and the three flag words were also confirmed at the same
 * offsets in Chapter 3's code.
 */
enum {
    SaveHeaderName = 0x00,          /* NUL-terminated, <= 24 chars */
    SaveHeaderFacing = 0x96,        /* u16, one of SaveFacing */
    SaveHeaderWorldX = 0x98,        /* u16 */
    SaveHeaderWorldY = 0x9A,        /* u16 */
    SaveHeaderGameDay = 0x9C,       /* u16 */
    SaveHeaderGameMonth = 0x9E,     /* u16 */
    SaveHeaderGameYear = 0xA0,      /* u16 */
    SaveHeaderClockMinutes = 0xA2,  /* u16 */
    SaveHeaderRoleAssignments = 0xA4, /* 5 x u16 */
    SaveHeaderGold = 0xB4,          /* Bcd4 */
    SaveHeaderOreCounter1 = 0xB8,   /* Bcd4 */
    SaveHeaderOreCounter2 = 0xBC,   /* Bcd4 */
    SaveHeaderPartySlots = 0x1EC    /* 4 x u16: 1-based party record id, 0 = empty */
};

/* g_partyFacing values (ShowCompassDirection, yendor2.asm:30608). */
typedef enum {
    SaveFacingNorth = 0x8000,
    SaveFacingSouth = 0x4000,
    SaveFacingEast = 0x1000,
    SaveFacingWest = 0x2000
} SaveFacing;

typedef struct {
    GameKind kind;
    uint8_t bytes[SaveFileSizeMax];
} SaveGame;

const SaveLayout *saveLayoutFor(GameKind kind);

/* A zero-filled image of the right size for a new game. */
void saveGameInit(SaveGame *save, GameKind kind);

/* Copies a whole file in; the kind is inferred from the exact size. */
bool saveGameLoad(SaveGame *save, const uint8_t *data, size_t size);

/* Copies the image out; returns the byte count, or 0 if capacity is too small. */
size_t saveGameStore(const SaveGame *save, uint8_t *out, size_t capacity);

/* A section's bytes, or NULL for an invalid section. */
uint8_t *saveGameSection(SaveGame *save, SaveSection section);

/* One record of a section, or NULL if the section or index is out of range. */
uint8_t *saveGameRecord(SaveGame *save, SaveSection section, unsigned index);

/* Party record by 0-based array index (0-8), NULL if out of range. */
uint8_t *saveGamePartyRecord(SaveGame *save, unsigned index);

/* Party record by the 1-based id used in the slot table (SelectPartyRecordById); NULL for 0 or > 9. */
uint8_t *saveGamePartyRecordById(SaveGame *save, unsigned id);

/* Little-endian accessors into the game-state block. */
uint16_t saveHeaderGetU16(const SaveGame *save, unsigned offset);
void saveHeaderSetU16(SaveGame *save, unsigned offset, uint16_t value);
uint8_t *saveHeaderBcd4(SaveGame *save, unsigned offset);

/* 1-based party record id in UI slot 0-3, or 0 if empty/out of range. */
uint16_t saveGetPartySlot(const SaveGame *save, unsigned slot);

/* Copies the save's name into out (always NUL-terminated). */
void saveGetName(const SaveGame *save, char out[SaveNameBufferSize]);

/*
 * Copies name plus its NUL over the start of the name field, leaving any
 * bytes after it untouched, exactly like the original. Returns false (and
 * changes nothing) if the name is longer than SaveNameMaxLength.
 */
bool saveSetName(SaveGame *save, const char *name);

const char *saveFacingName(uint16_t facing); /* "NORTH", ..., or NULL if not a facing value */

/* "SAVGAME1".."SAVGAME6"; false for other slot numbers. */
bool saveSlotFileName(char out[SaveSlotFileNameSize], unsigned slot);

#define SAVE_CURRENT_FILE_NAME "CURGAME"

#endif
