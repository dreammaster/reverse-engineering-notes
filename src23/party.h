#ifndef YENDOR23_PARTY_H
#define YENDOR23_PARTY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * Party-member records: the 500-byte structures at g_partyRecords (nine per
 * save, stored in section 1 of CURGAME right after the game-state block; see
 * savegame.h). All accessors take a pointer to one record's first byte and
 * use little-endian u16 fields. Field meanings were checked against the
 * disassembly and against the four real Chapter 2 characters.
 *
 * Stats are stored in pairs: the value at offset X is the current value and
 * the copy at X + 0x40 is the maximum/natural value (e.g. HP 0x52/0x92).
 *
 * Chapter 3 uses the same offsets: its inventory/equipment lookup, class
 * tables and stat-name table index the record identically. It differs only
 * in a few names and one equipment-slot quirk (see partyStatName and
 * partyEquipmentSlot). Chapter 3 has no local save file to check data against.
 */

enum {
    PartyRecordSize = 500,
    PartyNameMaxLength = 13,
    PartyNameBufferSize = PartyNameMaxLength + 1,
    PartyProtectionCount = 9,
    PartyMaxStatOffset = 0x40 /* distance from a current stat to its maximum */
};

typedef enum {
    PartyFieldName = 0x00,        /* NUL-terminated, <= 13 chars */
    PartyFieldClass = 0x0E,       /* u16 class id, see partyClassName */
    PartyFieldGender = 0x10,      /* u16: 1 or 2 (the two real female characters are 2) */
    PartyFieldUnknown12 = 0x12,   /* u16, values 20-33 in real data; meaning not identified */
    PartyFieldPortrait = 0x14,    /* u16 portrait icon id */
    PartyFieldLevel = 0x16,       /* u16, capped at 90 by training items */
    PartyFieldExperience = 0x18,  /* Bcd4 */
    PartyFieldStatusFlags = 0x1C, /* u16, PartyStatus bits */
    PartyFieldProtections = 0x20, /* 9 x u16 resistance values, order of PartyProtection */
    PartyFieldStats = 0x3C,       /* 27 x u16 current values, indexed by PartyStat */
    PartyFieldStatsMax = 0x7C,    /* 27 x u16 maximum values, same indexing */
    PartyFieldAbilities = 0xB4,   /* u16 bitmask of learned special abilities (0x8000..0x1000) */
    PartyFieldAbilityCharge = 0xB6, /* 4 x u16 charge counters, one per ability bit, high bit first */
    PartyFieldWearMain = 0xBE,    /* u16 wear counter for equipment slot 0xA (main weapon) */
    PartyFieldWearSecond = 0xC0,  /* u16 wear counter for equipment slot 0xC */
    PartyFieldWearThird = 0xC2,   /* u16 wear counter for equipment slot 0xD */
    PartyFieldFlagBankCA = 0xCA,  /* 16 words = 256 flags: known spells/abilities */
    PartyFieldFlagBank10C = 0x10C, /* 6 words = 96 flags: per-character one-time events */
    PartyFieldInventory = 0x118,  /* main InventoryGroup (34 bytes) */
    PartyFieldEquipment = 0x13A,  /* equipment slots, see partyEquipmentSlot */
    PartyFieldUiFlags = 0x15C,    /* u16, PartyUiFlags: which inventory group is displayed etc. */
    PartyFieldBagMarkers = 0x17C  /* 3 open-bag records of 0x26 bytes, see partyBagMarker */
} PartyField;

/* Indices into the stat arrays at PartyFieldStats / PartyFieldStatsMax (the game's 27-entry name table). */
typedef enum {
    PartyStatStrength,
    PartyStatDexterity,
    PartyStatStamina,
    PartyStatIntelligence,
    PartyStatWisdom,
    PartyStatCharisma,
    PartyStatEquipRating1, /* five equipment-derived ratings (0x48-0x50); unnamed in the game's table */
    PartyStatEquipRating2,
    PartyStatEquipRating3,
    PartyStatEquipRating4,
    PartyStatEquipRating5,
    PartyStatHitPoints,
    PartyStatMagicPoints,
    PartyStatCarryCapacity, /* 10 x Strength at creation; unnamed in the table */
    PartyStatSurvival,
    PartyStatProjectile,
    PartyStatSlashing,
    PartyStatBashing,
    PartyStatPolearm,
    PartyStatCasting,
    PartyStatMapping,
    PartyStatNavigation,
    PartyStatBartering,
    PartyStatRepair,
    PartyStatThievery,
    PartyStatLinguistics,
    PartyStatChemistry,
    PartyStatCount
} PartyStat;

typedef enum {
    PartyStatusSecondaryClassMask = 0x003F, /* one bit per secondary class, see partyClassSecondaryBit */
    PartyStatusDead = 0x0040,
    PartyStatusCursed = 0x0080,
    PartyStatusHexed = 0x0100,
    PartyStatusJinxed = 0x0200,
    PartyStatusStoned = 0x0400,
    PartyStatusFrozen = 0x0800,
    PartyStatusParalyzed = 0x1000,
    PartyStatusDiseased = 0x2000,
    PartyStatusPoisoned = 0x4000,
    PartyStatusSick = 0x8000,
    PartyStatusIncapacitated = 0x1C40 /* dead, stoned, frozen or paralyzed: can't act */
} PartyStatus;

/* Order of the nine values at PartyFieldProtections. */
typedef enum {
    PartyProtectionDisease,
    PartyProtectionPoison,
    PartyProtectionSickness,
    PartyProtectionStoning,
    PartyProtectionFrozen,
    PartyProtectionParalyze,
    PartyProtectionCursing,
    PartyProtectionHexing,
    PartyProtectionJinxing
} PartyProtection;

/* PartyFieldUiFlags bits set by GetInventorySlotPtr to record what the inventory screen is showing. */
typedef enum {
    PartyUiEquipmentShort = 0x0040, /* last slot looked up was a 2-byte equipment slot */
    PartyUiMainGroup = 0x0080,
    PartyUiBag3 = 0x0100,
    PartyUiBag2 = 0x0200,
    PartyUiBag1 = 0x0400
} PartyUiFlags;

uint16_t partyGetU16(const uint8_t *record, unsigned offset);
void partySetU16(uint8_t *record, unsigned offset, uint16_t value);

void partyGetName(const uint8_t *record, char out[PartyNameBufferSize]);

/* Current and maximum value of a stat; 0 for an out-of-range stat. */
uint16_t partyGetStat(const uint8_t *record, PartyStat stat);
uint16_t partyGetStatMax(const uint8_t *record, PartyStat stat);
void partySetStat(uint8_t *record, PartyStat stat, uint16_t value);
void partySetStatMax(uint8_t *record, PartyStat stat, uint16_t value);

/*
 * The game's name for a stat ("STRENGTH", ...), or NULL for the unnamed ones.
 * Chapter 3 calls hit points "HEALTH" and has no name for the chemistry
 * skill (its slot is still in the record).
 */
const char *partyStatName(PartyStat stat, GameKind game);

uint16_t partyGetProtection(const uint8_t *record, PartyProtection protection);

uint8_t *partyExperience(uint8_t *record); /* Bcd4 */

/*
 * Class ids are tier * 10 + base: base 1-9 (FIGHTER..MARKSMAN), tier 0-2.
 * Valid ids are 1-9, 11-19 and 21-29; the game's own table lookup gives
 * garbage for 0, 10 and 20 (GetClassNameString, yendor2.asm:16564).
 */
bool partyClassIsValid(unsigned classId);
unsigned partyClassBase(unsigned classId);
unsigned partyClassTier(unsigned classId);
const char *partyClassName(unsigned classId); /* NULL if invalid */

/* Status bit recording that a character reached secondary class 4-9 (tier 0 only); 0 otherwise. */
uint16_t partyClassSecondaryBit(unsigned classId);

/*
 * Flag banks: index 1..(words*16), 1-based and most-significant-bit first
 * (GetGlobalFlagBitAndWord, yendor2.asm:42355). Index 0 and out-of-range
 * indices test false and are ignored on set/clear.
 */
bool flagBankTest(const uint8_t *bank, unsigned words, unsigned index);
void flagBankSet(uint8_t *bank, unsigned words, unsigned index);
void flagBankClear(uint8_t *bank, unsigned words, unsigned index);

bool partyTestAbilityFlag(const uint8_t *record, unsigned index);
void partySetAbilityFlag(uint8_t *record, unsigned index);
bool partyTestEventFlag(const uint8_t *record, unsigned index);
void partySetEventFlag(uint8_t *record, unsigned index);

/*
 * An inventory group is 34 bytes: a u16 total carried weight followed by 8
 * item slots of 4 bytes (u16 item id, u16 extra word; id 0 = empty). The
 * same layout is used by a character's main inventory, an open bag, and
 * every 34-byte item-instance record in the save file (section 3).
 */
enum {
    InventoryGroupSize = 34,
    InventorySlotCount = 8,
    ItemSlotSize = 4
};

typedef enum {
    PartyGroupMain,
    PartyGroupBag1,
    PartyGroupBag2,
    PartyGroupBag3,
    PartyGroupCount
} PartyGroup;

uint8_t *partyInventoryGroup(uint8_t *record, PartyGroup group);

/*
 * The group the inventory screen acts on: the first open bag in priority
 * order (bag 1, 2, 3), otherwise the main inventory (GetInventorySlotPtr).
 */
uint8_t *partyActiveInventoryGroup(uint8_t *record);

uint16_t inventoryGroupWeight(const uint8_t *group);
void inventoryGroupSetWeight(uint8_t *group, uint16_t weight);

/* Slot 1-8, or NULL. */
uint8_t *inventoryGroupSlot(uint8_t *group, unsigned slot);

uint16_t itemSlotId(const uint8_t *slot);
uint16_t itemSlotExtra(const uint8_t *slot);
void itemSlotSet(uint8_t *slot, uint16_t id, uint16_t extra);

/*
 * Equipment slots use the game's command codes 0xA-0x14. 0xA-0xF are 4-byte
 * slots (id + extra) at 0x13A, 0x13E, ... 0x14E; 0x10-0x14 are 2-byte,
 * id-only slots at 0x152, 0x154, ... 0x15A. NULL for other codes.
 *
 * Chapter 3's lookup has no case for code 0x12, so it returns the same
 * slot as 0x14 (0x15A) and slot 0x156 can't be reached through it.
 */
enum {
    PartyEquipmentFirst = 0x0A,
    PartyEquipmentFirstShort = 0x10,
    PartyEquipmentLast = 0x14
};
uint8_t *partyEquipmentSlot(uint8_t *record, unsigned code, GameKind game);

/*
 * An open bag: [+0] container item id (0 = closed), [+2] the record number
 * of that container's contents in the save's item-instance table, [+4] the
 * 34-byte contents (an InventoryGroup) held while the bag is open. Bag 0-2.
 */
enum { PartyBagMarkerSize = 0x26 };
uint8_t *partyBagMarker(uint8_t *record, unsigned bag);

#endif
