/* The readable rules (rules.c) as hosts for the recompiled game: they replace the translated routines of the same names.
 * tests/test_rules_diff.c checks them against the translated originals. */
#include "game.h"
#include "rules.h"

static Mm3Rules rules(void) { Mm3Rules r = { DG }; return r; }
static const Mm3Character *far_char(Cpu *c, int n) { return (const Mm3Character *)(SEGP(host_arg(c, n + 1)) + host_arg(c, n)); }

void host_statBonus(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_stat_bonus(&r, host_arg(c, 0)); }
void host_getAge(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_char_age(&r, far_char(c, 0), host_arg(c, 2) != 0); }
void host_itemScan(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_item_scan(&r, far_char(c, 0), (int16_t)host_arg(c, 2)); }
void host_conditionMod(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_condition_mod(&r, far_char(c, 0), (int16_t)host_arg(c, 2)); }
void host_getStat(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_char_stat(&r, far_char(c, 0), (int16_t)host_arg(c, 2), host_arg(c, 3) != 0); }
void host_getCurrentLevel(Cpu *c) { c->ax = (uint16_t)mm3_char_level(far_char(c, 0)); }
void host_getMaxHP(Cpu *c) { Mm3Rules r = rules(); c->ax = mm3_max_hp(&r, far_char(c, 0)); }
void host_getMaxSP(Cpu *c) { Mm3Rules r = rules(); c->ax = mm3_max_sp(&r, far_char(c, 0)); }
/* getArmorClass(near character pointer, base_only) */
void host_getArmorClass(Cpu *c) { Mm3Rules r = rules(); c->ax = (uint16_t)mm3_armor_class(&r, (const Mm3Character *)(DG + host_arg(c, 0)), host_arg(c, 1) != 0); }

/* the overlay-callable thunks j_X jump to X */
void host_j_getMaxHP(Cpu *c) { host_getMaxHP(c); }
void host_j_getMaxSP(Cpu *c) { host_getMaxSP(c); }
void host_j_getArmorClass(Cpu *c) { host_getArmorClass(c); }
