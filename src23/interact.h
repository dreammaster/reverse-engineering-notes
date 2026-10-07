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
 * curgameIdOffset is exactly the lock bitmap's size: 608 in Chapter 2 (`_val10`, yendor2.asm:3467) and 1008 in Chapter 3 (`word_2ECF8` = DS:0x0F48, set
 * by Chapter 3's InitGlobals with the raw operand `mov word ptr ds:0F48h, 3F0h`). **Correction (2026-10-07): an earlier version of this note, and the code, said
 * Chapter 3's was "read but never written, always 0"** -- IDA does not attribute the raw `ds:` writes of that InitGlobals to the named words, which made every
 * Chapter 3 global initialised there look unwritten (this one, the curgame record multiplier word_3320E = DS:0x545E = 1000, and the class-promotion thresholds
 * word_331F8/FA = DS:0x5448/0x544A = 10/30; yendor3/ida_scripts/check_curgame_globals.py verifies the address arithmetic).
 */
bool interactBitmapTest(SaveGame *save, unsigned bitIndex);
void interactBitmapSet(SaveGame *save, unsigned bitIndex);

/* 0-based bit index for a 1-based lock id (see lockcatalog.h's lockCatalogRecord). */
unsigned interactLockBitIndex(unsigned lockId);

/* Chapter 2: 608 (lockcatalog.h's LockRecordCountYendor2). Chapter 3: 1008 (see the note above). */
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
 * curgameFlags is the first word of the 4-byte EMS record
 * LoadCurgameRecord reads (see lockcatalog.h's "LoadCurgameRecord"
 * note) -- confirmed 2026-09-26 to be nothing more exotic than
 * g_lockStatusFlags, the same global LoadLockState populates for an
 * ordinary lock (a CURGAME "trigger" record and a lock record are the
 * same physical shape, just read through two different loaders). Its
 * bits' own meaning in *this* dispatch context still isn't resolved
 * beyond the priority order TryInteractAtPosition tests them in
 * (0x10, then 0x8, then 0x40, then 0x20 as a tiebreaker between the
 * two fallback outcomes) -- see party.h's partyDecodeSavingThrowEffect
 * for the *second* word's own, now fully-resolved meaning (a packed
 * saving-throw threshold + effect id, consumed by a wholly different
 * caller, UseAbilityCommand/ApplySavingThrowEffect, not this one).
 */
InteractOutcome interactClassifyCurgame(uint16_t curgameFlags, bool alreadyTriggered);

/*
 * Key matching (UnlockDoorCommand, yendor2.asm:45970, instruction-identical in
 * Chapter 3 -- the handler for the key items, ids 0x21-0x2F). The key item's
 * consumable-target entry (item.h) starts with a flags word:
 *   high byte  the tier bit (0x80 BRASS ... 0x02 GOLD -- the same bits as
 *              lockcatalog.h's LockFlagKey* shifted down 8)
 *   0x80       a chest key        0x40  a door key        0x20  a master key (KEY RING, word 0x0030)
 * Real data: BRASS CHEST KEY 0x8090, BRONZE 0x4090, ... GOLD CHEST KEY 0x0290;
 * BRASS DOOR KEY 0x8050 ... GOLD DOOR KEY 0x0250; KEY RING 0x0030.
 *
 * interactKeyOpens decides whether the key opens the faced target, whose flags
 * word (a lock-catalog record's, or a CURGAME record's -- they are the same
 * shape) names the required tier(s) in its high byte:
 *   lockTarget (the 0x8000 object path, LoadLockState): a master key uses
 *     heldKeyFlags (the ring's accumulated tiers) HIGH byte; any other key must
 *     be a chest key (0x80) and supplies its own word's high byte.
 *   otherwise (the 0x4000 path, LoadCurgameRecord): a master key uses
 *     heldKeyFlags' LOW byte; any other key must be a door key (0x40) and supplies
 *     its own word's high byte.
 * Success = that byte shares a bit with targetFlags' high byte. (heldKeyFlags
 * only ever has low-byte bits -- see interactKeyRingContribution -- so a master
 * key never opens the lockTarget path in practice; reproduced.)
 */
bool interactKeyOpens(bool lockTarget, uint16_t keyWord0, uint16_t heldKeyFlags, uint16_t targetFlags);

/*
 * FinishPlacingHeldItem's accumulation (:50205): placing an item adds its tier
 * byte to g_heldKeyFlags (low byte), except an item with flag 0x80 (a chest key)
 * adds nothing -- so the ring remembers door keys only.
 */
uint16_t interactKeyRingContribution(uint16_t keyWord0);

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

/*
 * The bitIndex interactBitmapTest/Set need for a world object: its own
 * lock id (interactLockBitIndex) if it's a door, its curgame id
 * (interactCurgameBitIndex) otherwise -- the exact same shared
 * "already unlocked/triggered" bitmap either kind of record uses (see
 * this header's own top-of-file note). object must select
 * InteractBranchLock or InteractBranchCurgame; other branches have no
 * bit to compute.
 */
unsigned interactWorldObjectBitIndex(GameKind game, const WorldObjectRecord *object);

/*
 * The shared "classify, and if the outcome is one of a caller-supplied
 * set, mark the matching bit" shape both of ApplyEncodedItemEffect's
 * word_33302 bit 0x1 and bit 0x40 branches reduce to (see
 * interactKnock below for bit 0x1's own fixed outcome set; bit 0x40,
 * yendor2.asm:51852, instruction-identical in Chapter 3, is this same
 * shape with qualifying = {InteractOutcomeLockFlag40,
 * InteractOutcomeLockPriced, InteractOutcomeCurgameFlag40} -- not
 * given its own named wrapper here since, unlike "Knock", this
 * project doesn't have a confident read on what unifies an unconfirmed
 * lock flag, a priced lock, and an unrelated curgame flag into one
 * spell/item effect; bit 0x40's own UI-only addition, a text-column
 * choice via ShowAbilityDescriptionColumn, isn't modeled either way).
 * Classifies via interactClassify (given lock/curgame state the
 * caller already loaded, matching that function's own contract --
 * loading either kind of record isn't wired up end-to-end yet, see
 * lockcatalog.h's "LoadCurgameRecord" note) and, only when the result
 * is in qualifying, marks the matching bit via interactBitmapSet --
 * the same write UnlockDoorCommand performs on an ordinary key-based
 * unlock. Returns false (no bitmap write) for InteractOutcomeNone or
 * any non-qualifying outcome, including object == NULL
 * (interactClassify's own InteractBranchNone case).
 */
bool interactResolveIfOutcome(SaveGame *save, GameKind game, const WorldObjectRecord *object, const LockRecord *lock,
                               bool lockAlreadyUnlocked, uint16_t curgameFlags, bool curgameAlreadyTriggered,
                               bool monsterAlreadySpawned, const InteractOutcome *qualifying,
                               unsigned qualifyingCount);

/*
 * ApplyEncodedItemEffect's word_33302 bit 0x1 branch (yendor2.asm:51797,
 * instruction-identical in Chapter 3) -- a "Knock"-style effect that
 * auto-unlocks a magically-locked door or resolves a curgame record's
 * fallback-B state at the world cell worldobjects.h's
 * worldObjectProbeFacingTile finds (the party's own cell, or the cell
 * one step ahead in their facing). interactResolveIfOutcome with
 * qualifying = {InteractOutcomeLockMagical, InteractOutcomeCurgameFallbackB}.
 */
bool interactKnock(SaveGame *save, GameKind game, const WorldObjectRecord *object, const LockRecord *lock,
                    bool lockAlreadyUnlocked, uint16_t curgameFlags, bool curgameAlreadyTriggered,
                    bool monsterAlreadySpawned);

/*
 * ApplyEncodedItemEffect's word_33302 bit 0x8 branch (yendor2.asm:51741,
 * instruction-identical in Chapter 3) -- the same probe-then-classify-
 * then-mark shape as interactKnock (worldObjectProbeFacingTile, then
 * interactClassify), but a third qualifying set:
 * {InteractOutcomeCurgameFlag10, InteractOutcomeCurgameFlag8} -- the two
 * *highest*-priority curgame flags (see TryInteractAtPosition's own
 * 0x10/0x8/0x40/0x20 test order, this header's top-of-file note), and
 * no lock outcome at all -- unlike Knock/bit 0x40, this branch can
 * never resolve a door, only a curgame record. Given its own name
 * rather than left anonymous since, unlike bit 0x40, its qualifying set
 * is at least internally consistent (both members are "a curgame flag
 * fired"); what the two flags themselves represent narratively still
 * isn't confirmed.
 */
bool interactTriggerFacingCurgameEvent(SaveGame *save, GameKind game, const WorldObjectRecord *object,
                                        const LockRecord *lock, bool lockAlreadyUnlocked, uint16_t curgameFlags,
                                        bool curgameAlreadyTriggered, bool monsterAlreadySpawned);

/*
 * UnlockDoorCommand (yendor2.asm:45970; Chapter 3 the same), after the key item has been chosen and the facing tile probed. The key's own target flags word
 * (`keyWord0`, see interactKeyOpens) and the key ring's accumulated tiers (`heldKeyFlags`) decide:
 *   - nothing found at the party's cell or the one ahead, or a record that is neither a door (0x8000) nor a curgame record (0x4000): InteractUnlockNothing
 *     (sound 3);
 *   - its "already unlocked / triggered" bit is set: InteractUnlockAlready (the "already unlocked" description);
 *   - the key opens it (interactKeyOpens with the lock's or curgame record's flags): the bit is set (the original also writes the section back to CURGAME)
 *     and `clearCellBits` is 0x4000 for the caller to clear in the faced grid cell's flags (the door marker, `and [cell+6], 0xBFFF`), InteractUnlockOpened;
 *   - otherwise InteractUnlockLocked: ShowLockStatus shows what the lock needs (lockRequiredKeyType).
 * `locks` is the catalog (lockcatalog.h, curgame records included).
 */
typedef enum { InteractUnlockNothing, InteractUnlockAlready, InteractUnlockOpened, InteractUnlockLocked } InteractUnlockResult;

typedef struct {
    InteractUnlockResult result;
    int worldCol, worldRow; /* the faced object's cell */
    uint16_t clearCellBits; /* to clear in that cell's grid flags (InteractUnlockOpened) */
} InteractUnlockOutcome;

InteractUnlockOutcome interactUnlockFacing(SaveGame *save, GameKind game, const WorldObjectTable *objects, const LockCatalog *locks, int partyCol, int partyRow,
                                            uint16_t facing, uint16_t keyWord0, uint16_t heldKeyFlags);

#endif
