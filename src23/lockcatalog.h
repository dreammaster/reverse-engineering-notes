#ifndef YENDOR23_LOCKCATALOG_H
#define YENDOR23_LOCKCATALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The static lock/door definition catalog: one 26-byte record per lock id
 * (the same 1-based id worldobjects.h's WorldObjectFlagDoor record's
 * `value` field carries), giving the key tier it requires and the
 * flags/price ShowLockStatus (yendor2.asm:12719, not reimplemented --
 * pure UI display) reads to choose its message.
 *
 * The original loads this catalog through EMS paging (LoadLockState,
 * yendor2.asm:12600), not a flat single-block read like every other
 * catalog in this project -- but the underlying WORLD.DAT bytes are a
 * perfectly ordinary flat array, so no paging needs reimplementing: the
 * catalog's offset is found the same way as every other resource block
 * (its own "resource block setup" stub, PrepareWorldDat2BlockRead), and
 * loadWorldDat2 immediately followed by loadWorldDat3 (both called once
 * from InitGame) turn out to read this same catalog in two back-to-back
 * chunks into the shared EMS buffer -- confirmed by loadWorldDat3's own
 * stub (PrepareWorldDat3BlockRead) reporting an offset exactly
 * loadWorldDat2's read size later in WORLD.DAT (0x7E5BA + 0x3CF0 =
 * 0x822AA for Chapter 2, checked arithmetically, not assumed).
 *
 * Confirmed fields, cross-referenced against LoadLockState's own copy
 * (13 words = 26 bytes, into scratch starting at g_lockStatusFlags) and
 * ShowLockStatus's message dispatch:
 *   +0 flags (u16) -- see LockFlag. Bit 0x20 ("magically locked") and
 *      the required-key-type bits 0x200-0x8000 (one of the 7 door-key
 *      items already confirmed by an earlier session, cross-checked
 *      against the Hex Hacking Item Guide's door-key item table) were
 *      already known to exist; this session pinned down the exact
 *      bit<->key mapping by resolving ShowLockStatus's message
 *      addresses against the real key-name string labels
 *      (yendor2/ida_scripts/check_lock_key_strings.py) -- a casual
 *      reading of the dispatch order alone gives the WRONG mapping
 *      (it looks like a Gold>Silver>...>Brass tier at a glance, but
 *      address verification shows 0x8000=BRASS, not GOLD). More than
 *      one of the 7 bits CAN be set on the same record -- rare in
 *      Chapter 2's real data (0/608), common in Chapter 3's (123/1008)
 *      -- so this is a genuine bitmask, not a one-hot selector;
 *      ShowLockStatus (and lockRequiredKeyType below) resolves a
 *      multi-bit record by testing Brass first, Gold last, and using
 *      whichever bit matches first. Bits 0x1/0x2/0x80 are real (common
 *      in both games' real data)
 *      but their exact meaning isn't confirmed -- ShowLockStatus
 *      branches on them but the branches only affect which of a few
 *      very similar messages is shown, not the return outcome. Bit
 *      0x40 (LockFlagUnknown40) is also real and tested by
 *      TryInteractAtPosition (interact.h) -- confirmed to select its
 *      errorCode=8 outcome there, but its own meaning is still not
 *      confirmed either.
 *   +2 price (u16) -- a plain binary value, displayed split by 100 into
 *      two denominations (LoadLockState: `word_32DD0 / 100`,
 *      `word_32DD0 % 100`); which currency isn't confirmed.
 *   +4..+25 (22 bytes) -- not yet traced by any function read so far;
 *      exposed as raw bytes, not interpreted.
 */

enum {
    LockRecordSize = 26,
    LockRecordCountYendor2 = 608, /* matches savegame.h's SaveSectionLockAndShopState recordCount */
    LockRecordCountYendor3 = 1008,
    LockRecordCountMax = LockRecordCountYendor3
};

typedef struct {
    uint32_t offset; /* WORLD.DAT byte offset of lock id 1's record */
    uint16_t recordCount;
} LockCatalogLayout;

const LockCatalogLayout *lockCatalogLayout(GameKind game);

typedef struct {
    GameKind game;
    uint16_t recordCount;
    uint8_t records[LockRecordCountMax * LockRecordSize];
} LockCatalog;

/* Parses recordCount*LockRecordSize bytes starting at region[0]; false if size is too small. */
bool lockCatalogParse(LockCatalog *catalog, GameKind game, const uint8_t *region, size_t size);

/* Same, from a whole WORLD.DAT image already in memory. */
bool lockCatalogParseWorldDat(LockCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

typedef enum {
    LockFlagMagical = 0x0020,
    LockFlagUnknown40 = 0x0040,
    LockFlagKeyGold = 0x0200,
    LockFlagKeySilver = 0x0400,
    LockFlagKeySteel = 0x0800,
    LockFlagKeyIron = 0x1000,
    LockFlagKeyCopper = 0x2000,
    LockFlagKeyBronze = 0x4000,
    LockFlagKeyBrass = 0x8000
} LockFlag;

typedef struct {
    uint16_t flags;
    uint16_t price;
    uint8_t rest[LockRecordSize - 4]; /* raw, uninterpreted +4..+25 */
} LockRecord;

/* 1-based lock id, matching worldobjects.h's door record value field. False if id is 0 or out of range. */
bool lockCatalogRecord(const LockCatalog *catalog, unsigned lockId, LockRecord *out);

/*
 * The key-type bit set on flags, checked in the same order and
 * priority ShowLockStatus itself uses (LockFlagKeyBrass first, down to
 * LockFlagKeyGold) -- when more than one of the 7 bits is set (common
 * in Chapter 3's real data, rare in Chapter 2's), Brass wins over the
 * others tested later. Returns 0 if none are set.
 */
uint16_t lockRequiredKeyType(uint16_t flags);

/* "BRASS KEY" .. "GOLD KEY" for one of the LockFlagKey* constants (as returned by lockRequiredKeyType), else NULL. */
const char *lockKeyTypeName(uint16_t keyFlag);

#endif
