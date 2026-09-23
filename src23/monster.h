#ifndef YENDOR23_MONSTER_H
#define YENDOR23_MONSTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * Monsters. A live monster is a 156-byte record: 50 bytes of runtime state
 * followed by a verbatim copy of its 106-byte catalog block from WORLD.DAT
 * (SpawnMonsterInFacingDirection loads the block straight to record + 0x32,
 * yendor2.asm:33008). The same record shape is used by the 80-entry
 * g_levelMonsters pool saved in CURGAME and by the 3 active combat slots.
 *
 * The catalog is a table of 106-byte blocks (index 0 is empty) followed by a
 * table of u16 values mapping a monster *type id* (the id stored on a map
 * cell and in record +0x00) to a block index. Layout and field meanings were
 * checked against both games' real WORLD.DAT files; the runtime prefix comes
 * from the code only, since the available saves hold no live monsters.
 *
 * All field offsets below are record offsets (catalog block offset + 0x32).
 */

enum {
    MonsterRecordSize = 156,
    MonsterBlockSize = 106,
    MonsterBlockOffset = 0x32, /* where the catalog block sits in a record */
    MonsterPoolSize = 80,      /* g_levelMonsters */
    MonsterActiveSlots = 3,    /* g_monsterSlots */

    MonsterBlockCountMax = 73,
    MonsterLookupCountMax = 2500,

    MonsterNameLineSize = 13,
    MonsterNameBufferSize = 28 /* two 12-character lines, a space, NUL, with room */
};

typedef enum {
    /* Runtime state (0x00-0x31). */
    MonsterFieldType = 0x00,      /* u16 type id; 0 = empty pool slot */
    MonsterFieldWorldX = 0x02,    /* u16 */
    MonsterFieldWorldY = 0x04,    /* u16 */
    MonsterFieldCell = 0x06,      /* u16 byte offset of its cell in the dungeon grid */
    MonsterFieldAnim = 0x08,      /* u16; starts at MonsterFieldSpriteBase + RandomInRange(5), i.e. 0-5 */
    MonsterFieldAnimSet = 0x0A,   /* u16; 0xA when MonsterFlagAltSprite is set, else 0xD */
    MonsterFieldState = 0x0C,     /* u16, MonsterState bits */
    MonsterFieldWound = 0x0E,     /* u16, MonsterWound bits -- despite the name, also carries the
                                      party-relative direction and ambush-pending state (see MonsterWound) */
    MonsterFieldHealth = 0x10,    /* u16 current hit points; also read/written by monsterTickTimer (see there) */
    MonsterFieldTarget = 0x12,    /* runtime pointer to the party member being attacked; meaningless on disk */
    MonsterFieldFlagOnDeath = 0x14, /* i16 global flag index set (>0) or cleared (<0) when it dies */
    MonsterFieldFlagOnDeath2 = 0x16, /* i16 second such flag */
    /* +0x18: referenced by nothing traced so far. */
    MonsterFieldTickTarget = 0x1A,   /* u16; zeroed by monsterTickTimer's reset step, not otherwise traced */
    MonsterFieldTickAmount = 0x1C,   /* u16 subtracted from MonsterFieldHealth once or twice per monsterTickTimer call */
    MonsterFieldTickCountdown = 0x1E, /* u16 ticks remaining until monsterTickTimer's state-machine reset fires */

    /* Catalog block (0x32-0x9B). */
    MonsterFieldName1 = 0x32,     /* 13-byte text lines, 12 characters + NUL */
    MonsterFieldName2 = 0x3F,
    MonsterFieldSpriteBase = 0x4C, /* u16 first picture id of its sprite */
    MonsterFieldUnknown4E = 0x4E, /* u16, 1-13 in real data; not identified */
    MonsterFieldMaxHealth = 0x50, /* u16 ("HEALTH-") */
    MonsterFieldSaveDifficulty = 0x52, /* u16 saving-throw DC for its effects */
    MonsterFieldAccuracy = 0x54,  /* u16 ("ACCURACY-") */
    MonsterFieldDexterity = 0x56, /* u16 ("DEXTERITY-"); also the turn-order initiative */
    MonsterFieldAbsorption = 0x58, /* u16 ("ABSORPTION-") */
    MonsterFieldDamage = 0x5A,    /* u16 ("DAMAGE-") */
    MonsterFieldHitSound = 0x5C,  /* u16 sound id when it hits */
    MonsterFieldIdleSound = 0x5E, /* u16 sound id when it has no target */
    MonsterFieldApproachGate = 0x60, /* u16; ProcessLevelMonsters skips its approach/ambush check entirely when this is 0 -- exact meaning not confirmed */
    MonsterFieldRangedAccuracy = 0x64, /* u16 ("RANGED ACC.-") */
    MonsterFieldRangedDamage = 0x66,   /* u16 ("RANGED DAM.-") */
    MonsterFieldAttackEffect = 0x6C,   /* u16 effect id (effect.h) of its ordinary attack; always an HP-cost effect */
    MonsterFieldSpecialAttack = 0x6E,  /* u16 effect id of its special attack ("SPECIAL ATTACK-"), 0 = none; chosen 25% of the time */
    MonsterFieldPalette = 0x72,   /* 3 x u16 colour remap, used when MonsterFlagRemapPalette is set */
    MonsterFieldLootGold = 0x7E,  /* Bcd4 */
    MonsterFieldLootNuore = 0x82, /* Bcd4 */
    MonsterFieldLootOre = 0x86,   /* Bcd4, magic ore */
    MonsterFieldExperience = 0x8A, /* Bcd4 */
    MonsterFieldFlags = 0x92,     /* u16, MonsterFlag bits */
    MonsterFieldAwareness = 0x94, /* u16, MonsterAwareness bits */
    MonsterFieldImmunities = 0x96, /* u16, MonsterImmunity bits */
    MonsterFieldResistances = 0x98 /* u16, MonsterResistance bits */
} MonsterField;

/* MonsterFieldState bits. */
typedef enum {
    MonsterStateAware = 0x0001, /* it has noticed the party (TryActivateMonsterByDistance); gates ProcessLevelMonsters */
    MonsterStateBusy = 0x0800   /* skips ProcessLevelMonsters' approach/ambush check this tick; exact trigger not confirmed */
} MonsterState;

/*
 * MonsterFieldWound bits (yendor2.asm:33367 on, ProcessLevelMonsters/
 * TriggerSideTrapForRandomPartyMember, instruction-identical in Chapter
 * 3). Despite the field's name, only 0x8000/0x4000/0x2000 are wound
 * severity -- the other bits are set by ProcessLevelMonsters' approach
 * check (see monsterApproachParty in monsterpool.h) and read by both it
 * and the side-trap/ambush presentation pipeline
 * (file-formats.md's "side trap"/ambush section).
 */
typedef enum {
    MonsterWoundLight = 0x8000,
    MonsterWoundModerate = 0x4000,
    MonsterWoundSevere = 0x2000,
    /*
     * Set by ProcessLevelMonsters' approach check to record the monster's
     * position relative to the party -- which way the party must be
     * facing to trigger the pending ambush/trap. Exactly one of these 4
     * is set after a successful alignment check (never combined).
     */
    MonsterWoundPartyMustFaceNorth = 0x0800, /* monster is north of the party */
    MonsterWoundPartyMustFaceSouth = 0x0400, /* monster is south of the party */
    MonsterWoundPartyMustFaceEast = 0x0200,  /* monster is east of the party */
    MonsterWoundPartyMustFaceWest = 0x0100,  /* monster is west of the party */
    /* An ambush/trap is armed and waiting for the party to face the direction above (ProcessSideTrapsOnMovement). */
    MonsterWoundAmbushPending = 0x1000
} MonsterWound;

/*
 * A second, independent bit range within MonsterFieldAwareness (not the
 * already-documented 0x20-0x100 "how far it notices the party" range):
 * selects the ambush-roll threshold ProcessLevelMonsters uses once a
 * monster has a clear line to the party (RandomInRange(100) must be <=
 * the threshold for the ambush to trigger). Tested highest-bit-first;
 * none of the 4 set falls back to a threshold of 5 (the lowest chance).
 */
typedef enum {
    MonsterAmbushChanceVeryHigh = 0x1000, /* threshold 90 */
    MonsterAmbushChanceHigh = 0x0800,     /* threshold 75 */
    MonsterAmbushChanceMedium = 0x0400,   /* threshold 50 */
    MonsterAmbushChanceLow = 0x0200       /* threshold 25 */
} MonsterAmbushChance;

/* MonsterFieldFlags bits (only those the code tests). */
typedef enum {
    MonsterFlagAltSprite = 0x0001,    /* alternate sprite layout: anim set 0xA (else 0xD), clue-book category 0x30 (else 0x20) */
    MonsterFlagRemapPalette = 0x0004, /* apply MonsterFieldPalette */
    MonsterFlagAreaAttack = 0x1000,   /* hits the whole party, not one target */
    MonsterFlagSpecialMask = 0x0E00   /* modifiers shown next to its special attack */
} MonsterFlag;

/*
 * MonsterFieldAwareness: gates when TryActivateMonsterByDistance (see
 * monsterTryActivateByDistance) sets MonsterStateAware. Zero means the
 * default (baseline threshold only); the other bits select the viewport
 * depth threshold that must be exceeded: 0x2C/44 (0x40), 0x29/41 (0x80)
 * or 0x26/38 (0x100); 0x20 means it never activates by distance at all.
 * Full priority order and the shared baseline gate (>0x21/33, checked
 * before any of these) are in monsterTryActivateByDistance's own doc
 * comment (monster.c), instruction-identical in both games.
 *
 * **Naming caveat, not resolved**: taken as literal "how close before it
 * notices you," the threshold ordering reads backwards from the names --
 * MonsterAwarenessFar requires the *highest* (closest) depth threshold
 * to activate, MonsterAwarenessNear the *lowest* (farthest). Checked a
 * hypothesis that this might describe preferred engagement range
 * instead (ranged/ambush types staying dormant, melee types waking
 * early) against Chapter 2's real catalog -- not supported: no real
 * Chapter 2 monster uses MonsterAwarenessFar at all, and the few that
 * use Near/Middle (CARNIVOROUS, FOREST GIANT, both OGRE entries,
 * SCAVENGER, SPIDER, GRIZZLY BEAR) are all melee types, not ranged
 * ones -- every ranged monster (CENTAUR MAGE, DARK MAGE, EVIL WIZARD,
 * HALFLING, WIZARD, ROGUE, THIEF, NECROMANCER, SEA DRAGON, ...) uses
 * the *default* tier (no Far/Middle/Near/Never bit) instead. So this
 * remains genuinely unresolved -- possibly just a backwards name from
 * whichever session first assigned it, possibly something else
 * entirely. Kept the existing names rather than guess at a rename.
 */
typedef enum {
    MonsterAwarenessNever = 0x0020,
    MonsterAwarenessFar = 0x0040,
    MonsterAwarenessMiddle = 0x0080,
    MonsterAwarenessNear = 0x0100
} MonsterAwareness;

/* MonsterFieldImmunities bits (the clue book's "IMMUNE" rows). */
typedef enum {
    MonsterImmuneMagicResist = 0x0010, /* also drives "MAGIC DAMAGE: RESISTANT" */
    MonsterImmunePoison = 0x8000,
    MonsterImmuneDisease = 0x4000,
    MonsterImmuneParalysis = 0x2000,
    MonsterImmuneFreezing = 0x1000,
    MonsterImmuneHexing = 0x0800,
    MonsterImmuneCursing = 0x0400,
    MonsterImmuneFire = 0x0008,
    MonsterImmuneCold = 0x0004,
    MonsterImmuneElectric = 0x0002,
    MonsterImmunePower = 0x0001
} MonsterImmunity;

/* MonsterFieldResistances bits (the clue book's "RESISTANT" rows). */
typedef enum {
    MonsterResistMagicMask = 0x3A00,
    MonsterResistPhysicalMask = 0xC000
} MonsterResistance;

typedef enum {
    MonsterLootGold,
    MonsterLootNuore,
    MonsterLootOre,
    MonsterLootExperience
} MonsterLoot;

typedef struct {
    uint32_t blocksOffset; /* WORLD.DAT offset; the lookup table follows the last block */
    uint16_t blockCount;
    uint16_t lookupCount;
    uint32_t totalSize;    /* blocks + lookup bytes */
} MonsterCatalogLayout;

typedef struct {
    GameKind game;
    uint16_t blockCount;
    uint16_t lookupCount;
    uint8_t blocks[MonsterBlockCountMax * MonsterBlockSize];
    uint8_t lookup[MonsterLookupCountMax * 2];
} MonsterCatalog;

const MonsterCatalogLayout *monsterCatalogLayout(GameKind game);

/* Parses the region starting at layout->blocksOffset; false if size is too small. */
bool monsterCatalogParse(MonsterCatalog *catalog, GameKind game, const uint8_t *region, size_t size);
bool monsterCatalogParseWorldDat(MonsterCatalog *catalog, GameKind game, const uint8_t *worldDat, size_t size);

/* Catalog block by index (0 is the empty block), or NULL if out of range. */
const uint8_t *monsterCatalogBlock(const MonsterCatalog *catalog, unsigned index);

/* Block index for a monster type id; 0 if the type is unknown or maps outside the catalog. */
unsigned monsterCatalogBlockIndex(const MonsterCatalog *catalog, unsigned typeId);

/*
 * Builds a new live record the way SpawnMonsterInFacingDirection does apart
 * from position, animation and randomness: zeroes it, stores the type id,
 * copies the catalog block, sets current health to maximum and applies the
 * type's on-death flag deltas. False (record untouched) if the type is unknown.
 */
bool monsterRecordSpawn(uint8_t *record, const MonsterCatalog *catalog, unsigned typeId);

/* Sets world position and the grid cell offset ((y - originRow) * 0x270 + (x - originCol) * 8). */
void monsterRecordPlace(uint8_t *record, uint16_t x, uint16_t y, uint16_t gridOriginRow, uint16_t gridOriginCol);

/* Sets the animation start to sprite base + randomExtra (RandomInRange(5), 0-5) and the anim set from MonsterFlagAltSprite. */
void monsterRecordStartAnimation(uint8_t *record, unsigned randomExtra);

uint16_t monsterGetU16(const uint8_t *record, unsigned offset);
void monsterSetU16(uint8_t *record, unsigned offset, uint16_t value);

const uint8_t *monsterLoot(const uint8_t *record, MonsterLoot kind); /* Bcd4 */

bool monsterIsImmune(const uint8_t *record, MonsterImmunity kind);
bool monsterResistsMagic(const uint8_t *record);
bool monsterResistsPhysical(const uint8_t *record);

/* Trimmed name lines. */
void monsterGetNameLine(const uint8_t *record, unsigned line, char out[MonsterNameLineSize]);

/*
 * The display name exactly as BuildMonsterDisplayName builds it: line 1, a
 * space, line 2, each trimmed first. A one-line name therefore ends in a
 * space ("ALLIGATOR "), as in the original.
 */
void monsterGetName(const uint8_t *record, char out[MonsterNameBufferSize]);

/*
 * The in-EXE table of monster types whose death sets or clears a global flag
 * (17 entries in Chapter 2, 23 in Chapter 3). False if the type has none.
 */
bool monsterDeathFlags(GameKind game, unsigned typeId, int16_t *flagA, int16_t *flagB);

/* The ambush-roll threshold (90/75/50/25/5) for a MonsterFieldAwareness value -- see MonsterAmbushChance. */
unsigned monsterAmbushThreshold(uint16_t awareness);

/*
 * TickMonsterTimer (yendor2.asm:33320, yendor3.asm:33098, instruction-
 * identical). A per-tick state machine gated on MonsterFieldState bits
 * 0xFC10 -- for a monster with none of those bits set (the common case),
 * this is a no-op returning MonsterTickIdle. When gated in, it subtracts
 * MonsterFieldTickAmount from MonsterFieldHealth (twice, if state bits
 * 0x3010 are also set) and, once MonsterFieldTickCountdown independently
 * reaches 0, resets the whole mechanism (clears state bits outside mask
 * 0x3ED, zeroes MonsterFieldTickTarget/Amount/Countdown, and resets
 * MonsterFieldAnim to MonsterFieldSpriteBase).
 *
 * What triggers these state bits in the first place, and therefore what
 * this mechanism actually represents (a status effect's duration? a
 * scripted despawn timer? something else), isn't traced -- neither
 * caller (ProcessLevelMonsters, ProcessMonsterAttackTurn) sets these
 * bits itself, only reads the result. Reimplemented faithfully as the
 * confirmed bit/arithmetic operations regardless.
 */
typedef enum {
    MonsterTickIdle = 0,     /* the 0xFC10 gate wasn't set; nothing happened */
    MonsterTickExpired = 1,  /* MonsterFieldHealth reached <= 0; ProcessLevelMonsters treats this as "remove the monster" */
    MonsterTickOngoing = 2   /* mid-decrement, still alive; ProcessMonsterAttackTurn skips this monster's attack either way */
} MonsterTickResult;

MonsterTickResult monsterTickTimer(uint8_t *record);

/*
 * TryActivateMonsterByDistance (yendor2.asm:34123, yendor3.asm:33917,
 * instruction-identical thresholds included). The setter for
 * MonsterStateAware -- everywhere else this session treats it as an
 * input, this is where it actually gets decided. A no-op if already
 * aware. Otherwise, a shared baseline gate (viewportDepth > 0x21/33)
 * must pass first; below that, nothing ever activates regardless of
 * MonsterFieldAwareness. Above it: MonsterAwarenessNever blocks
 * activation outright; otherwise the highest-priority tier bit that's
 * set (Far, then Middle, then Near) picks a stricter threshold the
 * depth must also exceed; with none of the 3 tier bits set, the
 * baseline alone is enough. Sets MonsterStateAware and returns true on
 * activation, false otherwise (including the already-aware case).
 *
 * viewportDepth is the same "viewport index" concept
 * monsterpool.h's MonsterSpawnOffsetCount table is indexed by
 * (SpawnMonsterInFacingDirection calls this right after placing a
 * monster using that same value) -- higher means closer to the party,
 * per that table's own confirmed shape, though what exactly it means
 * at the moment *this* function is called (mid-render-sweep state, not
 * necessarily "the monster's current distance") isn't fully pinned
 * down.
 */
bool monsterTryActivateByDistance(uint8_t *record, uint16_t viewportDepth);

#endif
