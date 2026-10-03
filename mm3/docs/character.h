/* MM3 character record, 303 (0x12F) bytes, little endian.  Offsets verified against getStat/getMaxHP/getMaxSP/
 * getArmorClass/itemScan/subtractHitPoints/ifProc/getAge/getCurrentExperience (see docs/overview.md).
 * "(Xeen)" = same position/meaning as the ScummVM Xeen Character class. */
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
	uint8_t permanent, temporary;
} Mm3AttrPair;

typedef struct {
	char    name[16];            /* 00 */
	uint8_t sex;                 /* 10 */
	uint8_t race;                /* 11 */
	uint8_t alignment;           /* 12 (ifProc action 6) */
	uint8_t charClass;           /* 13: 0 knight 1 paladin 2 archer 3 cleric 4 sorcerer 5 robber 6 ninja 7 barbarian 8 druid 9 ranger */
	Mm3AttrPair stat[7];         /* 14: might, intellect, personality, endurance, speed, accuracy, luck */
	uint8_t acTemp;              /* 22 (Xeen _ACTemp) */
	uint8_t level;               /* 23 permanent level */
	uint8_t tempLevel;           /* 24 */
	uint8_t birthDay;            /* 25 */
	uint8_t tempAge;             /* 26 */
	uint8_t skills[18];          /* 27: thievery, arms master, astrologer, bodybuilder, ... (Xeen order) */
	uint8_t awards[26];          /* 39: ifProc action 15 / hasAward; fewer than Xeen's 64 */
	uint8_t spells[36];          /* 53: ifProc action 19 loops 24h entries */
	uint8_t lloydMap, lloydX, lloydY; /* 77 */
	uint8_t hasSpells;           /* 7A (getMaxSP) */
	uint8_t currentSpell;        /* 7B (guess, Xeen order) */
	uint8_t quickOption;         /* 7C (guess, Xeen order) */
	/* 18 inventory slots, stored as parallel arrays 19 bytes apart (loops run 0..17) */
	uint8_t slotPresent[19];     /* 7D non-zero = slot used (Xeen item._frame) */
	uint8_t slotFlags[19];       /* 90 40h cursed, 80h broken (itemScan tests C0h) */
	uint8_t slotElement[19];     /* A3 elemental material -> ELEMENTAL_RESISTANCES */
	uint8_t slotMetal[19];       /* B6 armour/metal material -> METAL_LAC */
	uint8_t slotAttribute[19];   /* C9 attribute enchantment material -> ATTRIBUTE_BONUSES */
	uint8_t slotId[19];          /* DC item id (armour 21h-29h, ARMOR_STRENGTHS) */
	uint8_t slotSpell[19];         /* EF item special-ability (spell) id used by castItemSpell, 4Dh = none */
	uint8_t blessed;             /* 102 AC bonus, set by Blessed, added by getArmorClass(base=false) */
	uint8_t powerShield;         /* 103 set by Power Shield */
	uint8_t holyBonus;           /* 104 damage bonus (Holy Bonus), added to METAL_DAMAGE in getWeaponDamage */
	uint8_t heroism;             /* 105 to-hit bonus (Heroism), added to METAL_DAMAGE_PERCENT */
	uint8_t unknown106[13];      /* 106 one byte, then six (temporary, permanent) resistance pairs at 107 fire, 109 cold, 10B electricity, 10D poison, 10F energy, 111 magic (charSavingThrow) */
	uint8_t conditions[16];      /* 113: 0 cursed ... 0Ch unconscious (11Fh) 0Dh dead (120h) ... (Xeen order) */
	uint16_t unknown123;         /* 123 */
	int16_t  hp;                 /* 125 */
	int16_t  sp;                 /* 127 */
	uint16_t birthYear;          /* 129 */
	uint32_t experience;         /* 12B */
} Mm3Character;
#pragma pack(pop)
_Static_assert(sizeof(Mm3Character) == 0x12F, "Mm3Character size");
/* Verified against MAZE.CHR of MM3.CUR with tools/mm3_chars.py (the 30 premade characters decode sensibly):
 * sex 0 male 1 female; race 0 human 1 elf 2 dwarf 3 gnome 4 half-orc; alignment 0 good 1 neutral 2 evil. */
