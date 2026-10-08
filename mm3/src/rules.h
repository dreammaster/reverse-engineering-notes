/* Character rules (stats, hit/spell points, armour class): readable C versions of routines that the translated game
 * code also contains (itemScan, conditionMod, getStat, getMaxHP, getMaxSP, getArmorClass ...).  tests/test_rules_diff.c fuzzes
 * them against the translated originals; the game uses these through the hosts in rules_host.c.
 * The tables are read from the game's data segment (DGROUP offsets as in docs/rules.md) so no game data is compiled in. */
#ifndef MM3_RULES_H
#define MM3_RULES_H

#include <stdint.h>
#include "character.h"

enum { MM3_STAT_MIGHT, MM3_STAT_INTELLECT, MM3_STAT_PERSONALITY, MM3_STAT_ENDURANCE, MM3_STAT_SPEED, MM3_STAT_ACCURACY, MM3_STAT_LUCK };

/* What the rules need besides the character: the data segment (tables, party year). */
typedef struct { const uint8_t *dg; } Mm3Rules;

int mm3_stat_bonus(const Mm3Rules *r, uint16_t value);              /* STAT_BONUSES[first i with STAT_VALUES[i] > value] */
int mm3_char_age(const Mm3Rules *r, const Mm3Character *ch, int ignore_temp);
int mm3_item_scan(const Mm3Rules *r, const Mm3Character *ch, int which); /* sum of equipment bonuses for stat/attribute `which` */
int mm3_condition_mod(const Mm3Rules *r, const Mm3Character *ch, int which); /* stat changes from conditions (weak, drunk, ...) */
int mm3_char_stat(const Mm3Rules *r, const Mm3Character *ch, int stat, int base_only);
int mm3_char_level(const Mm3Character *ch);                         /* permanent + temporary level, not below 0 */
uint16_t mm3_max_hp(const Mm3Rules *r, const Mm3Character *ch);
uint16_t mm3_max_sp(const Mm3Rules *r, const Mm3Character *ch);
int mm3_armor_class(const Mm3Rules *r, const Mm3Character *ch, int base_only);

#endif
