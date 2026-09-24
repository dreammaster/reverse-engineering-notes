#ifndef YENDOR23_INTERACT_H
#define YENDOR23_INTERACT_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "lockcatalog.h"
#include "savegame.h"
#include "worldobjects.h"

/*
 * TryInteractAtPosition (yendor2.asm:30813, instruction-identical in
 * Chapter 3): the per-cell interaction dispatcher run on every movement
 * step, plus a handful of other callers (UnlockDoorCommand, the
 * map-editor debug overlay, click-to-travel). Looks up the cell's
 * worldobjects.h record, then classifies it into one of the
 * InteractOutcome values below -- the real `start` main loop uses this
 * to decide whether to autosave CURGAME, and ShowLockStatus /
 * HandleSpecialCellEntry (neither reimplemented here, pure UI/rendering)
 * read it to choose what to show the player.
 *
 * Two side effects of the original are deliberately not modeled here:
 * g_uiScratchFlags3 bit 0x80 (a UI "redraw dirty" marker set on the
 * monster-spawn branch) and g_uiScratchFlags3 bit 0x80 being cleared
 * unconditionally on entry -- both belong to the rendering layer.
 */

/*
 * The shared "already resolved" bitmap this session traced LoadLockState
 * and LoadCurgameRecord (lockcatalog.h) both reading, and
 * UnlockDoorCommand (yendor2.asm:45970) writing back on a successful
 * unlock -- CURGAME's SaveSectionEventState (savegame.h), previously
 * only documented as generic "byte-addressed state" without knowing it
 * was bit-packed or shared between two id spaces:
 *
 *   - bits [0, lockRecordCount) -- one bit per lock id (see
 *     lockcatalog.h), 0-based (lockId - 1), MSB-first within its byte
 *     (byte = bitIndex/8, bit = 7 - bitIndex%8 -- the same convention
 *     already confirmed for the explored-map and monster-spawn
 *     bitmaps). Set once a door has been unlocked.
 *   - bits [curgameIdOffset, curgameIdOffset + N) -- one bit per
 *     curgame-record id (the value field of a WorldObjectFlagCurgameRecord
 *     record), same 0-based/MSB-first packing, offset by
 *     curgameIdOffset so the two id spaces don't collide. Set once
 *     that record's one-time effect has been triggered.
 *
 * curgameIdOffset is exactly Chapter 2's lock count (608, `_val10`,
 * yendor2.asm:56832) -- confirmed arithmetically, not assumed. **In
 * Chapter 3 the equivalent global (`word_2ECF8`) is read but never
 * written anywhere in the disassembly, always 0** -- the same
 * always-zero-global quirk already found for LoadCurgameRecord's EMS
 * record multiplier (`word_3320E`, see lockcatalog.h's "LoadCurgameRecord"
 * note). With curgameIdOffset == 0, Chapter 3's curgame-record ids
 * collide with its own lowest lock ids' unlock bits -- not determined
 * whether that's a genuine bug, dead code, or the two id spaces simply
 * never overlap in practice for Chapter 3's real data.
 */
bool interactBitmapTest(SaveGame *save, unsigned bitIndex);
void interactBitmapSet(SaveGame *save, unsigned bitIndex);

/* 0-based bit index for a 1-based lock id (see lockcatalog.h's lockCatalogRecord). */
unsigned interactLockBitIndex(unsigned lockId);

/* Chapter 2: 608 (lockcatalog.h's LockRecordCountYendor2). Chapter 3: 0 (see the always-zero-global note above). */
unsigned interactCurgameIdOffset(GameKind game);

/* 0-based bit index for a 1-based curgame-record id (a WorldObjectFlagCurgameRecord record's value field). */
unsigned interactCurgameBitIndex(GameKind game, unsigned curgameId);

/* Which of TryInteractAtPosition's branches a cell's object record selects -- tested in exactly this priority order. */
typedef enum {
    InteractBranchNone,
    InteractBranchLock,           /* WorldObjectFlagDoor */
    InteractBranchCurgame,        /* WorldObjectFlagCurgameRecord */
    InteractBranchFixedResponse,  /* WorldObjectFlagFixedResponse */
    InteractBranchMonsterSpawn    /* WorldObjectFlagMonsterSpawn */
} InteractBranch;

/* NULL (no record found at this cell) selects InteractBranchNone. */
InteractBranch interactSelectBranch(const WorldObjectRecord *object);

/*
 * The numeric values below are TryInteractAtPosition's own `errorCode`
 * values, kept identical so this can be cross-referenced against the
 * disassembly directly. Several are still only "known to be reached
 * under condition X", not "known to mean Y" -- documented per case.
 */
typedef enum {
    InteractOutcomeNone = 0,             /* nothing here, or already resolved (unlocked/triggered/already-spawned) */
    InteractOutcomeCurgameFallbackA = 1, /* curgame record; none of flags 0x10/0x8/0x40/0x20 set */
    InteractOutcomeCurgameFallbackB = 2, /* curgame record; flag 0x20 set, no higher-priority bit */
    InteractOutcomeLockMagical = 3,      /* door; LockFlagMagical set */
    InteractOutcomeFixedResponse = 4,    /* WorldObjectFlagFixedResponse record; meaning not confirmed */
    InteractOutcomeUnspawnedMonster = 5, /* WorldObjectFlagMonsterSpawn record, not yet spawned */
    InteractOutcomeCurgameFlag10 = 6,    /* curgame record; flag 0x10 set */
    InteractOutcomeCurgameFlag8 = 7,     /* curgame record; flag 0x8 set */
    InteractOutcomeLockFlag40 = 8,       /* door; LockFlagUnknown40 set, meaning not confirmed */
    InteractOutcomeLockPriced = 9,       /* door; not magical, LockFlagUnknown40 clear, LockRecord.price != 0 */
    InteractOutcomeCurgameFlag40 = 10    /* curgame record; flag 0x40 set */
} InteractOutcome;

/* Priority order: LockFlagMagical, then LockFlagUnknown40, then a nonzero price. */
InteractOutcome interactClassifyLock(const LockRecord *lock, bool alreadyUnlocked);

/*
 * curgameFlags is the first word of the still-undecoded 4-byte EMS
 * record LoadCurgameRecord reads (see lockcatalog.h's "LoadCurgameRecord"
 * note) -- its bits' own meaning isn't resolved, only the priority order
 * TryInteractAtPosition tests them in (0x10, then 0x8, then 0x40, then
 * 0x20 as a tiebreaker between the two fallback outcomes).
 */
InteractOutcome interactClassifyCurgame(uint16_t curgameFlags, bool alreadyTriggered);

/* alreadySpawned: monsterSpawnFlagTest(save, game, object->value) (monsterpool.h). */
InteractOutcome interactClassifyMonsterSpawn(bool alreadySpawned);

/*
 * Full dispatch matching TryInteractAtPosition exactly. object is the
 * cell's worldobjects.h record (NULL if worldObjectFind found nothing).
 * lock/lockAlreadyUnlocked and curgameFlags/curgameAlreadyTriggered/
 * monsterAlreadySpawned are only read for the branch interactSelectBranch
 * picks -- pass zeroed/false for whichever don't apply.
 */
InteractOutcome interactClassify(const WorldObjectRecord *object, const LockRecord *lock, bool lockAlreadyUnlocked,
                                  uint16_t curgameFlags, bool curgameAlreadyTriggered, bool monsterAlreadySpawned);

#endif
