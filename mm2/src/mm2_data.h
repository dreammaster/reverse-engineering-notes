/* Decoded game tables: items, monsters, spells; and the 130-byte character record
 * (docs/file-formats.md).  Ports of tools/mm2_dump.py and the spell table notes in docs/spells.md. */
#ifndef MM2_DATA_H
#define MM2_DATA_H

#include "mm2_files.h"

/* ---- items (ITEMS.DAT, 256 x 20 bytes) ---- */
typedef struct {
	char name[13];
	uint8_t classMask;   /* class c cannot use the item when bit (7-c) is set */
	uint8_t bonus;       /* high nibble attribute 0-5, low nibble amount; F0h = not equippable */
	uint8_t useEffect;   /* 0 = cannot be used */
	uint16_t value;      /* weapon damage / armour class bonus */
	uint16_t price;
} Mm2Item;

#define MM2_ITEMS 256
int mm2_load_items(const Mm2Game *g, Mm2Item items[MM2_ITEMS]);

/* Item category by id (2CMDS range tests). */
typedef enum { MM2_ITEM_ONEHAND, MM2_ITEM_TWOHAND, MM2_ITEM_MISSILE, MM2_ITEM_SHIELD, MM2_ITEM_ARMOUR, MM2_ITEM_HELM, MM2_ITEM_MISC } Mm2ItemKind;
Mm2ItemKind mm2_item_kind(int id);

/* ---- monsters (MONSTERS.DAT, 256 x 26 bytes) ---- */
typedef struct {
	char name[15];
	int hp, exp, ac, speed, blows, damageDie, groupSize, picture;
	int magicResistPct, castChancePct, spell, touch, specialUses;
	int itemClass, goldClass, dropsGems;
	int bribeFood, bribeGold, bribeGems;
	int noSteal, ranged, undead, summoner, verb;
	int immune[8];       /* index = element 1..7 (1 fire, 2 electricity, 3 cold, 4 acid) */
} Mm2Monster;

#define MM2_MONSTERS 256
int mm2_load_monsters(const Mm2Game *g, Mm2Monster mons[MM2_MONSTERS]);

/* ---- spells (SPELLS.DAT, 96 x 2 bytes; 0-47 sorcerer list, 48-95 cleric list) ---- */
typedef struct {
	int usage;      /* 0 anytime, 1 combat only, 2 non-combat only */
	int gems;       /* above 50 is capped to 100 */
	int spCost;     /* 0 = special formula */
	int perLevel;   /* multiplier of caster level (0-3) */
	int locRestrict;
} Mm2Spell;

#define MM2_SPELLS 96
int mm2_load_spells(const Mm2Game *g, Mm2Spell spells[MM2_SPELLS]);

/* ---- characters (ROSTER.DAT: 48 x 82h bytes + state block) ---- */
#define MM2_CHAR_SIZE 0x82
#define MM2_ROSTER_CHARS 48
typedef struct {
	uint8_t raw[MM2_CHAR_SIZE];
} Mm2Char;

enum {
	MC_NAME = 0x00, MC_TOWN = 0x0B, MC_SEX = 0x0C, MC_ORIG_ALIGN = 0x0D, MC_RACE = 0x0E, MC_CLASS = 0x0F,
	MC_BASE_STATS = 0x10, MC_RESIST = 0x16, MC_THIEVERY = 0x1E, MC_BASE_LEVEL = 0x20, MC_AGE = 0x21, MC_DAY = 0x22,
	MC_BASE_SPELL_LEVEL = 0x23, MC_AC = 0x24, MC_FOOD = 0x25, MC_CONDITION = 0x26, MC_BASE_ENDURANCE = 0x27,
	MC_EQUIP_ID = 0x28, MC_EQUIP_FLAGS = 0x34, MC_PACK_ID = 0x3A, MC_PACK_FLAGS = 0x46, MC_SKILLS = 0x50,
	MC_SPELL_BITS = 0x51, MC_SP = 0x58, MC_SP_MAX = 0x5A, MC_GEMS = 0x5C, MC_HP = 0x5E, MC_EXP = 0x62, MC_GOLD = 0x66,
	MC_ALIGN = 0x6A, MC_CUR_STATS = 0x6B, MC_LEVEL = 0x71, MC_SPELL_LEVEL = 0x72, MC_ENDURANCE = 0x73, MC_HP_MAX = 0x74
};

static inline unsigned mm2_c8(const Mm2Char *c, int off) { return c->raw[off]; }
static inline unsigned mm2_c16(const Mm2Char *c, int off) { return c->raw[off] | (c->raw[off + 1] << 8); }
static inline uint32_t mm2_c32(const Mm2Char *c, int off) {
	return c->raw[off] | (c->raw[off + 1] << 8) | (c->raw[off + 2] << 16) | ((uint32_t)c->raw[off + 3] << 24);
}

typedef struct {
	Mm2Char chars[MM2_ROSTER_CHARS];
	uint8_t state[2052];
} Mm2Roster;

int mm2_load_roster(const Mm2Game *g, Mm2Roster *r);

#endif
