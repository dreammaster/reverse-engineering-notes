/* Numeric game tables copied from the EXE (generated: mm2_tables_gen.c) and rule formulas
 * (docs/classes.md, docs/combat.md).  Port of tools/mm2_rules.py. */
#ifndef MM2_TABLES_H
#define MM2_TABLES_H

#include <stdint.h>

extern const uint32_t MM2_EXP_TABLE[18];       /* [group 0/1][level 2..10] */
extern const uint16_t MM2_TOWN_MULT[5];
extern const uint16_t MM2_TOWN_HP_DIV[5];
extern const uint16_t MM2_HP_PER_LEVEL[8];
extern const uint8_t MM2_SWING_DIV[8];
extern const uint8_t MM2_HIT_DIV[8];
extern const uint8_t MM2_MONSTER_TOHIT[16];
extern const uint8_t MM2_STAT_BRACKET[23];

extern const uint8_t MM2_RACE_STAT_ADJ[35];
extern const uint8_t MM2_RACE_RESIST[48];
extern const uint8_t MM2_START_THIEVERY[8];
extern const uint16_t MM2_START_HP[8];
extern const uint16_t MM2_ENDURANCE_HP[22];
extern const uint16_t MM2_START_SP[23];
extern const uint8_t MM2_START_AC[15];
extern const uint8_t MM2_START_ITEM[64];

enum { MM2_KNIGHT, MM2_PALADIN, MM2_ARCHER, MM2_CLERIC, MM2_SORCERER, MM2_ROBBER, MM2_NINJA, MM2_BARBARIAN };

/* Experience needed to reach `level` (>= 2) for a class. */
uint32_t mm2_exp_for_level(int cls, int level);
/* Gold to train to `level` in town 0-4. */
uint32_t mm2_training_cost(int town, int level);
/* lookup_bracket (1354A): number of table entries below stat, minus 3. */
int mm2_bracket(int stat);

#endif
