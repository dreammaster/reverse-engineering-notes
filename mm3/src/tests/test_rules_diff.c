/* test_rules_diff DGROUP.BIN [N]: fuzz the readable rules (rules.c) against the translated originals (gen/rules_gen.c)
 * on random characters; the game data segment supplies the tables for both. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../recomp.h"
#include "../rules.h"
#include "../rules_host.h"

extern const RecompEntry recomp_entries_rules_gen[];

static uint32_t rs = 12345;
static unsigned rnd(unsigned n) { rs = rs * 1664525u + 1013904223u; return (rs >> 8) % n; }

#define CH_OFF 0x5000
static uint32_t call(const char *name, int nargs, const uint16_t *args) {
	Cpu c;
	const RecompEntry *e;
	if (getenv("RD_VERBOSE")) fprintf(stderr, " call %s(%04X %04X %04X %04X)\n", name, nargs > 0 ? args[0] : 0, nargs > 1 ? args[1] : 0, nargs > 2 ? args[2] : 0, nargs > 3 ? args[3] : 0);
	for (e = recomp_entries_rules_gen; e->name && strcmp(e->name, name); e++) {}
	if (!e->name) { fprintf(stderr, "no %s\n", name); exit(2); }
	memset(&c, 0, sizeof c);
	c.sp = STACK_TOP; c.ds = DSEG;
	for (int i = nargs - 1; i >= 0; i--) PUSH(&c, args[i]);
	PUSH(&c, 0); PUSH(&c, 0);
	e->fn(&c);
	return c.ax | ((uint32_t)c.dx << 16);
}

int main(int argc, char **argv) {
	FILE *f;
	unsigned n = argc > 2 ? (unsigned)atoi(argv[2]) : 2000, bad = 0, checks = 0;
	if (argc < 2 || !(f = fopen(argv[1], "rb")) || fread(DG, 1, 65536, f) != 65536) return 2;
	fclose(f);
	mm3_rules_set_dialog_answer(1);
	Mm3Rules rules = { DG };
	Mm3Character *ch = (Mm3Character *)(DG + CH_OFF);
	for (unsigned t = 0; t < n; t++) {
		if (getenv("RD_VERBOSE")) fprintf(stderr, "case %u\n", t);
		for (unsigned i = 0; i < sizeof *ch; i++) ((uint8_t *)ch)[i] = (uint8_t)(rnd(4) ? 0 : rnd(256)); /* sparse bytes */
		ch->race = rnd(5); ch->charClass = rnd(10); ch->level = 1 + rnd(40); ch->tempLevel = rnd(4) ? 0 : (uint8_t)(rnd(60) - 30);
		for (int s = 0; s < 7; s++) { ch->stat[s].permanent = 3 + rnd(60); ch->stat[s].temporary = rnd(3) ? 0 : (uint8_t)(rnd(20) - 8); }
		ch->hasSpells = rnd(3) != 0; ch->birthYear = 440 + rnd(60); ch->tempAge = rnd(3) ? 0 : rnd(30);
		for (int i = 0; i < 18; i++) {
			ch->slotPresent[i] = rnd(3) ? 1 + rnd(14) : 0; ch->slotFlags[i] = rnd(6) ? 0 : (uint8_t)(rnd(4) << 6);
			ch->slotElement[i] = rnd(3) ? 0 : 1 + rnd(36); ch->slotMetal[i] = rnd(3) ? 0 : 1 + rnd(12);
			ch->slotAttribute[i] = rnd(3) ? 0 : 1 + rnd(72); ch->slotId[i] = rnd(0x40);
		}
		for (int i = 0; i < 16; i++) ch->conditions[i] = rnd(5) ? 0 : (i >= 13 ? rnd(2) * (rnd(8) == 0) : 1 + rnd(5));
		ch->blessed = rnd(3) ? 0 : rnd(10); ch->acTemp = rnd(3) ? 0 : rnd(8); ch->skills[3] = rnd(2); ch->skills[2] = rnd(2); ch->skills[0x0C] = rnd(2); ch->skills[0x0D] = rnd(2);
		wr16(DG, 0xEC36, 500 + rnd(60)); /* Party_year */
		uint16_t p[4] = { CH_OFF, DSEG, 0, 0 };
#define CHECK(label, expect, name, nargs, ...) do { uint16_t a[4] = { __VA_ARGS__ }; uint32_t got = call(name, nargs, a) & 0xFFFF; checks++; \
	if ((uint16_t)(expect) != got) { if (bad++ < 10) printf("MISMATCH %s case %u: readable %04X translated %04X\n", label, t, (uint16_t)(expect), got); } } while (0)
		for (int s = 0; s < 7; s++) {
			CHECK("itemScan", mm3_item_scan(&rules, ch, s), "itemScan", 3, p[0], p[1], s);
			CHECK("conditionMod", mm3_condition_mod(&rules, ch, s), "conditionMod", 3, p[0], p[1], s);
			CHECK("getStat", mm3_char_stat(&rules, ch, s, 0), "getStat", 4, p[0], p[1], s, 0);
			CHECK("getStat base", mm3_char_stat(&rules, ch, s, 1), "getStat", 4, p[0], p[1], s, 1);
		}
		for (int w = 7; w < 16; w++) {
			CHECK("itemScan", mm3_item_scan(&rules, ch, w), "itemScan", 3, p[0], p[1], w);
			CHECK("itemScan(w)", mm3_item_scan(&rules, ch, w), "itemScan", 3, p[0], p[1], w);
		}
		CHECK("getAge0", mm3_char_age(&rules, ch, 0), "getAge", 3, p[0], p[1], 0);
		CHECK("getAge1", mm3_char_age(&rules, ch, 1), "getAge", 3, p[0], p[1], 1);
		CHECK("getCurrentLevel", mm3_char_level(ch), "getCurrentLevel", 2, p[0], p[1]);
		CHECK("getMaxHP", mm3_max_hp(&rules, ch), "getMaxHP", 2, p[0], p[1]);
		CHECK("getMaxSP", mm3_max_sp(&rules, ch), "getMaxSP", 2, p[0], p[1]);
		CHECK("getArmorClass0", mm3_armor_class(&rules, ch, 0), "getArmorClass", 2, CH_OFF, 0);
		CHECK("getArmorClass1", mm3_armor_class(&rules, ch, 1), "getArmorClass", 2, CH_OFF, 1);
		for (unsigned v = 0; v < 300; v += 1 + rnd(40)) CHECK("statBonus", mm3_stat_bonus(&rules, v), "statBonus", 1, v);
	}
	printf("%u checks, %u mismatches\n", checks, bad);
	return bad != 0;
}
