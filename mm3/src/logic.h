/* Game logic rewritten as readable C.  Every function here replaces a routine of the translated game (src/gen/game_gen.c); it works on
 * the game's data segment (`dg`, DGROUP offsets as in docs/ and names/) and is verified against the translated original with
 * `mm3game --difftest` (game_diff.c).  game_logic.c exposes them to the translated code as hosts. */
#ifndef MM3_LOGIC_H
#define MM3_LOGIC_H

#include <stdint.h>
#include "character.h"

typedef struct { uint8_t *dg; uint8_t *mem; /* flat real-mode memory: far pointer seg:off = mem + seg*16 + off */ } Mm3Game;

/* DGROUP offsets */
enum {
	MM3_DG_PARTY_CHARS = 0xB9D6,      /* the party's character records, 0x12F bytes each */
	MM3_DG_PARTY_STATE_BASE = 0xE8EA, /* Party_stateBase: party size (byte) */
	MM3_DG_ENGINE_MODE = 0xC520,      /* Engine_mode: 1 exploring, 2 combat ... */
	MM3_DG_COMBAT_PARTY_SIZE = 0xACC1,
	MM3_DG_COMBAT_ORDER = 0xECC9,     /* party member index per combat slot */
	MM3_DG_CLASS_XP = 0x1CB2,         /* words by class: experience base per level */
	/* the maze: four 16x16 pages ("slots") of 0x340 bytes: 256 cell words, 256 flag bytes, a 0x40-byte header (neighbour map ids at +8 / +9,
	 * automap "visited" bits at +0x20) */
	MM3_DG_MAZE_PAGES = 0xC554,
	MM3_DG_MAZE_SLOT_IDS = 0x274E,    /* map id held by each slot */
	MM3_DG_MAZE_SLOT_X = 0x2600,      /* x/y of each slot's page origin in the 32x32 window around the party (bytes) */
	MM3_DG_MAZE_SLOT_Y = 0x2604,
	MM3_DG_MAZE_CUR_SLOT = 0xC53E,    /* Maze_curSlot */
	MM3_DG_MAZE_WRAP_MODE = 0x15B,    /* Maze_wrapMode: non-zero on maps that wrap around */
	MM3_DG_BITSET_MASKS = 0x16F8,     /* mazeSetBits: word mask per field */
	MM3_DG_BITSET_SHIFTS = 0x159C,    /* ... and shift (byte, 0x58 bytes per field) */
	MM3_DG_PARTY_DEAD_FLAG = 0x151,   /* byte_28841: set by checkPartyDead when no member can act */
	MM3_DG_MONSTER_ROWS = 0xC4A2,     /* byte_34B92..94: monsters present in the first three rows ahead (also: groups in combat) */
	MM3_DG_COMBAT_GONE = 0xB9C9,      /* per combat slot: has acted this round */
	/* combat globals */
	MM3_DG_COMBAT_TARGET = 0x4B7C,
	MM3_DG_COMBAT_HIT_BONUS = 0x92FA,
	MM3_DG_COMBAT_WEAPON_DAMAGE = 0xABBA, /* word */
	MM3_DG_WEAPON_ELEMENT = 0xABB4,       /* byte_332A4: element of the attacking weapon */
	MM3_DG_WEAPON_SPELL = 0xC509,         /* byte_34BF9 */
	MM3_DG_WEAPON_DICE = 0xC4C9,          /* byte_34BB9 */
	MM3_DG_WEAPON_SIDES = 0xEC88,         /* byte_37378 */
	MM3_DG_MON_AC = 0xF07E,               /* far pointers to the loaded monster stat columns: offset word, segment word */
	MM3_DG_MON_COLUMN_OFFSET = 0xB6BC,    /* word per combat target: offset of its entry in each column */
	MM3_DG_MON_ASLEEP = 0xB810,           /* word per combat target (non-zero: easy to hit) */
	MM3_DG_WEAPON_HIT_BONUS = 0xA71,      /* by weapon metal */
	MM3_DG_WEAPON_METAL_DAMAGE = 0xA88,
	MM3_DG_WEAPON_DICE_COUNT = 0xCD0,     /* by item id */
	MM3_DG_WEAPON_DICE_SIDES = 0xD19,
	MM3_DG_SPELL_ATTACK_TYPE = 0xE60E,    /* word */
	MM3_DG_ELEMENT_RESIST_BASE = 0xA4C,   /* by weapon element */
	MM3_DG_MON_PHYS = 0xF07A, MM3_DG_MON_MAGI = 0xF05A, MM3_DG_MON_FIRE = 0xF046, MM3_DG_MON_ELEC = 0xF04A,
	MM3_DG_MON_COLD = 0xF04E, MM3_DG_MON_ACID = 0xF052, MM3_DG_MON_ENER = 0xF062,
	MM3_DG_SPELL_SP_COST = 0x1B7A,        /* words by spell: spell points (<= 0: that many per level) */
	MM3_DG_SPELL_GEM_COST = 0x1C16,
	MM3_DG_PARTY_GEMS = 0xEC58,           /* 32 bits: word at +0 and +2 */
	/* monster movement */
	MM3_DG_MON_Y = 0xAD70, MM3_DG_MON_X = 0xAEC4,  /* words per combat/maze monster slot */
	MM3_DG_MON_GRID = 0x8EF4,             /* 32x32 bytes: how much of each cell is occupied */
	MM3_DG_MON_SIZE = 0x1B20,             /* by monster type offset */
	MM3_DG_MON_MOVED = 0xED74,
	MM3_DG_MONSTERS_MOVE_FLAG = 0x15C, MM3_DG_MONSTERS_SEEN_FLAG = 0x14D
};

#define MM3_NO_SLOT 0x1111

Mm3Character *mm3_party_member(const Mm3Game *g, unsigned index);

/* experience (docs/rules.md "training grounds") */
uint32_t mm3_experience_total(const Mm3Game *g, const Mm3Character *ch);       /* getCurrentExperience */
uint32_t mm3_experience_for_next_level(const Mm3Game *g, const Mm3Character *ch); /* nextExperienceLevel */
uint32_t mm3_experience_needed(const Mm3Game *g, const Mm3Character *ch);       /* experienceToNextLevel: 0 when already eligible */
void mm3_give_experience(const Mm3Game *g, uint32_t amount);                       /* giveExperience: split evenly between the party */

/* maze cell access (coordinates relative to the party's page; see docs/data-files.md for the cell word layout) */
int mm3_maze_neighbour_slot(const Mm3Game *g, unsigned map_id);                /* mazeNeighbourSlot: slot holding that map, or MM3_NO_SLOT */
unsigned mm3_maze_word(const Mm3Game *g, int x, int y, unsigned mask);       /* mazeGetWordRel (0 / 1111h when off the map) */
unsigned mm3_maze_word_wrapped(const Mm3Game *g, int x, int y, unsigned mask); /* mazeGetWordWrap */
unsigned mm3_maze_flags(const Mm3Game *g, int x, int y, unsigned mask);      /* mazeGetFlagsRel */
void mm3_maze_set_bits(const Mm3Game *g, int x, int y, unsigned field, unsigned value); /* mazeSetBits */
void mm3_maze_mark_visited(const Mm3Game *g, int x, int y);                  /* markCellVisited */
unsigned mm3_maze_is_visited(const Mm3Game *g, int x, int y);                /* isCellVisited */
void mm3_set_bit(uint8_t *bits, unsigned index, int value);                  /* setBit: bit 0 is the top bit of byte 0 */
unsigned mm3_is_bit_set(const uint8_t *bits, unsigned index);                /* isBitSet */

/* party condition checks */
unsigned mm3_worst_condition(const Mm3Character *ch);                          /* worstCondition: index of the highest condition that is set, 16 = none */
void mm3_check_party_dead(const Mm3Game *g);                                   /* checkPartyDead: sets byte_28841 */
int mm3_all_have_gone(const Mm3Game *g);                                       /* allHaveGone: every active combat participant has acted */
int mm3_chars_cant_act(const Mm3Game *g);                                      /* charsCantAct: every character is asleep/paralysed/unconscious ... */
void mm3_subtract_hit_points(const Mm3Game *g, Mm3Character *ch, int amount);  /* subtractHitPoints */

/* combat rolls */
void mm3_weapon_damage(const Mm3Game *g, const Mm3Character *ch, int ranged);  /* getWeaponDamage: fills the Combat_* globals */
int mm3_hit_monster(const Mm3Game *g, const Mm3Character *ch, int ranged);     /* hitMonster: does the attack hit the current target? */
int mm3_saving_throw(const Mm3Game *g, const Mm3Character *ch, int kind);      /* charSavingThrow: 0 luck, 1 magic, 2 fire, 3 electricity, 4 cold, 5 poison, 6 energy */

/* character creation (docs/rules.md) */
void mm3_check_classes(const int8_t stats[7], uint8_t available[10]);          /* checkClasses: which classes the stats qualify for */
void mm3_roll_attributes(int8_t stats[7], uint8_t available[10]);              /* rollAttributes: three rounds of rnd(10,79)/10 per stat, then checkClasses */
int mm3_thievery(const Mm3Game *g, const Mm3Character *ch);                    /* getThievery: chance to pick locks etc. */

/* items */
uint32_t mm3_item_price(const Mm3Game *g, const Mm3Character *ch, int slot, int mode, int discount); /* itemPrice: 1 buy, 2 sell, 3-6 repair/identify fee, 0 nothing */

int mm3_monster_resistance(const Mm3Game *g, int kind);                         /* getMonsterResistance: scaled resistance of the current target */
int mm3_spend_spell_cost(const Mm3Game *g, Mm3Character *ch, int spell);       /* Spells_subSpellCost: 0 paid, 1 not enough spell points, 2 not enough gems */
void mm3_move_monster_by(const Mm3Game *g, int dx, int dy, int monster);       /* moveMonsterBy */

#endif
