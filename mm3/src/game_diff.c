/* `mm3game ../data --difftest [NAME] [N]`: checks every readable replacement (logic.c, listed in gen/readable.txt) against the translated
 * original (gen/ref_gen.c) on the live game state, with random changes to the state and the arguments.  Both versions start from the same
 * data segment and random seed; results (ax, dx) and the whole data segment (except the stack area) must match. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "logic.h"

extern const RecompEntry recomp_entries_ref_gen[];
void mm3_rng_seed(uint32_t seed);
uint32_t mm3_rng_get(void);

#define H(name) void host_##name(Cpu *c)
H(mazeNeighbourSlot); H(mazeGetWordRel); H(mazeGetWordWrap); H(mazeGetFlagsRel); H(mazeSetBits); H(markCellVisited); H(isCellVisited); H(setBit); H(isBitSet);
H(getMonsterResistance); H(Spells_subSpellCost); H(moveMonsterBy); H(itemPrice); H(checkClasses); H(rollAttributes); H(getThievery); H(getWeaponDamage); H(hitMonster); H(charSavingThrow); H(worstCondition); H(checkPartyDead); H(allHaveGone); H(charsCantAct); H(subtractHitPoints);
H(getCurrentExperience); H(nextExperienceLevel); H(experienceToNextLevel); H(giveExperience);

static uint32_t rs = 1;
static unsigned rnd(unsigned n) { rs = rs * 1664525u + 1013904223u; return (rs >> 8) % n; }

/* ---- setup helpers: a random party member / random arguments */
static void randomize_party(void) {
	for (int i = 0; i < 6; i++) {
		Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F);
		ch->charClass = rnd(10); ch->level = 1 + rnd(30); ch->experience = rnd(4) ? rnd(2000000) : rnd(1000);
	}
	DG[0xE8EA] = 1 + rnd(6);
	DG[0xC520] = rnd(2) ? 2 : 1; DG[0xACC1] = 1 + rnd(6);
	for (int i = 0; i < 8; i++) DG[0xECC9 + i] = rnd(6);
}
static void setup_char(unsigned iter, uint16_t *a) { (void)iter; randomize_party(); a[0] = 0xB9D6 + rnd(6) * 0x12F; }
static void setup_xp(unsigned iter, uint16_t *a) { (void)iter; randomize_party(); uint32_t v = rnd(3) ? rnd(100000) : rnd(0xFFFFFF); a[0] = (uint16_t)v; a[1] = (uint16_t)(v >> 16); }

/* a random maze window: four slots with distinct map ids, headers pointing at loaded neighbours, random cells/flags/visited bits */
static void randomize_maze(void) {
	for (int s = 0; s < 4; s++) DG[0x274E + s] = 40 + s * 3 + rnd(2) * 20 + (rnd(5) == 0 ? 5 : 0);
	for (int s = 0; s < 4; s++) { DG[0x2600 + s] = rnd(2) * 16; DG[0x2604 + s] = rnd(2) * 16; }
	DG[0xC53E] = rnd(4); DG[0x15B] = rnd(2);
	for (unsigned i = 0; i < 4 * 0x340; i++) DG[0xC554 + i] = (uint8_t)rnd(256);
	for (int s = 0; s < 4; s++) {
		DG[0xC554 + s * 0x340 + 0x308] = DG[0x274E + rnd(4)]; DG[0xC554 + s * 0x340 + 0x309] = DG[0x274E + rnd(4)];
		if (rnd(8) == 0) DG[0xC554 + s * 0x340 + 0x308] = 99; /* a neighbour that is not loaded */
	}
	if (rnd(3) == 0) { int a = 45 + rnd(8); DG[0x274E + DG[0xC53E]] = a; DG[0x274E + rnd(4)] = 44 + rnd(10); }
}
static void setup_xy(unsigned iter, uint16_t *a) { (void)iter; randomize_maze(); a[0] = (uint16_t)(int16_t)((int)rnd(40) - 4); a[1] = (uint16_t)(int16_t)((int)rnd(40) - 4); a[2] = rnd(2) ? 0xFFFF : rnd(0x10000); }
static void setup_xy_in(unsigned iter, uint16_t *a) {
	(void)iter; randomize_maze();
	for (int s = 0; s < 4; s++) { DG[0xC554 + s * 0x340 + 0x308] = DG[0x274E + rnd(4)]; DG[0xC554 + s * 0x340 + 0x309] = DG[0x274E + rnd(4)]; } /* all neighbours loaded */
	a[0] = rnd(32); a[1] = rnd(32); a[2] = rnd(4); a[3] = rnd(2);
}
static void setup_visit(unsigned iter, uint16_t *a) { (void)iter; randomize_maze(); a[0] = (uint16_t)(int16_t)((int)rnd(40) - 4); a[1] = (uint16_t)(int16_t)((int)rnd(40) - 4); }
static void setup_bit(unsigned iter, uint16_t *a) { (void)iter; for (int i = 0; i < 64; i++) DG[0x5000 + i] = (uint8_t)rnd(256); a[0] = 0x5000; a[1] = rnd(300); a[2] = rnd(3); }
static void setup_nb(unsigned iter, uint16_t *a) { (void)iter; randomize_maze(); a[0] = 40 + rnd(40); }

static void randomize_conditions(void) {
	randomize_party();
	for (int i = 0; i < 8; i++) {
		Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F);
		for (int k = 0; k < 16; k++) ch->conditions[k] = rnd(4) ? 0 : (rnd(3) == 0 ? 1 + rnd(3) : 0);
		if (rnd(3) == 0) ch->conditions[8 + rnd(8)] = 1;
		ch->hp = (int16_t)((int)rnd(60) - 30); ch->level = 1 + rnd(20); ch->race = rnd(5);
		for (int s = 0; s < 18; s++) { ch->slotId[s] = rnd(0x40); ch->slotPresent[s] = rnd(2); ch->slotFlags[s] = rnd(4) ? 0 : 0x40; }
		for (int s = 0; s < 7; s++) ch->stat[s].permanent = 3 + rnd(40);
		ch->birthYear = 440 + rnd(60);
	}
	DG[0xE8EA] = 1 + rnd(6); DG[0xACC1] = 1 + rnd(6);
	for (int i = 0; i < 16; i++) { DG[0xB9C9 + i] = rnd(3) ? 0 : 1; DG[0xECC9 + i] = rnd(6); }
	for (int i = 0; i < 3; i++) DG[0xC4A2 + i] = rnd(3) ? 0 : 1 + rnd(5);
	DG[0xEC36] = 500; DG[0xEC37] = 2; DG[0x151] = rnd(2);
}
static void setup_conditions(unsigned iter, uint16_t *a) { (void)iter; (void)a; randomize_conditions(); }
static void setup_char_cond(unsigned iter, uint16_t *a) { (void)iter; randomize_conditions(); a[0] = 0xB9D6 + rnd(6) * 0x12F; }
static void setup_combat(unsigned iter, uint16_t *a) {
	(void)iter; randomize_conditions();
	for (int i = 0; i < 6; i++) {
		Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F);
		for (int s = 0; s < 18; s++) {
			ch->slotPresent[s] = rnd(3) ? (uint8_t[]){ 1, 4, 0xD, 2, 0, 3 }[rnd(6)] : 0; ch->slotMetal[s] = rnd(12); ch->slotId[s] = 1 + rnd(0x40);
			ch->slotElement[s] = rnd(20); ch->slotSpell[s] = rnd(0x4E);
		}
		ch->heroism = rnd(5); ch->holyBonus = rnd(5); ch->conditions[0] = rnd(4) ? 0 : rnd(5);
	}
	/* the monster stat column: 64 bytes at a free spot in far memory */
	for (int i = 0; i < 64; i++) MEM[0x90000 + 0x4000 + i] = (uint8_t)rnd(60);
	wr16(DG, 0xF07E, 0x4000); wr16(DG, 0xF080, 0x9000);
	for (int t = 0; t < 8; t++) { wr16(DG, 0xB6BC + t * 2, rnd(60)); wr16(DG, 0xB810 + t * 2, rnd(3) ? 0 : 1); }
	DG[0x4B7C] = rnd(8);
	a[0] = 0xB9D6 + rnd(6) * 0x12F; a[1] = rnd(2);
}
static void setup_save(unsigned iter, uint16_t *a) { setup_combat(iter, a); a[1] = rnd(7); }
static void setup_stats(unsigned iter, uint16_t *a) { (void)iter; for (int i = 0; i < 7; i++) DG[0x5100 + i] = (uint8_t)(rnd(5) ? 5 + rnd(14) : rnd(256)); for (int i = 0; i < 10; i++) DG[0x5200 + i] = (uint8_t)rnd(256); a[0] = 0x5100; a[1] = 0x5200; }
static void setup_thief(unsigned iter, uint16_t *a) { setup_combat(iter, a); for (int i = 0; i < 6; i++) { Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F); ch->skills[0] = rnd(3) ? 1 : 0; ch->charClass = rnd(10); ch->race = rnd(5); } a[1] = 0; }
static void setup_price(unsigned iter, uint16_t *a) {
	setup_combat(iter, a);
	for (int i = 0; i < 6; i++) { Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F); for (int s = 0; s < 18; s++) { ch->slotId[s] = rnd(0x58); ch->slotMetal[s] = rnd(14); ch->slotElement[s] = rnd(37); ch->slotAttribute[s] = rnd(73); ch->slotSpell[s] = rnd(0x4E); ch->slotFlags[s] = rnd(3) ? rnd(0x40) : (uint8_t)rnd(256); } }
	a[1] = rnd(18); a[2] = rnd(8); a[3] = rnd(4) ? rnd(0x20) : rnd(0x200);
}
static void setup_resist(unsigned iter, uint16_t *a) {
	setup_combat(iter, a);
	for (int col = 0; col < 8; col++) { uint16_t at = (uint16_t[]){ 0xF07A, 0xF05A, 0xF046, 0xF04A, 0xF04E, 0xF052, 0xF062, 0xF07E }[col]; wr16(DG, at, 0x4000 + col * 64); wr16(DG, at + 2, 0x9000); }
	for (int i = 0; i < 8 * 64; i++) MEM[0x90000 + 0x4000 + i] = rnd(3) ? (uint8_t)rnd(101) : 0;
	DG[0xABB4] = rnd(40); wr16(DG, 0xE60E, rnd(9)); DG[0x4B7C] = rnd(8);
	for (int e = 0; e < 40; e++) DG[0xA4C + e] = rnd(60);
	a[0] = rnd(5);
}
static void setup_spellcost(unsigned iter, uint16_t *a) {
	setup_combat(iter, a);
	for (int s = 0; s < 80; s++) { wr16(DG, 0x1B7A + s * 2, rnd(2) ? rnd(30) : (uint16_t)(-(int)rnd(4))); wr16(DG, 0x1C16 + s * 2, rnd(2) ? 0 : rnd(10)); }
	wr16(DG, 0xEC58, rnd(20)); wr16(DG, 0xEC5A, rnd(3) ? 0 : rnd(2));
	for (int i = 0; i < 6; i++) { Mm3Character *ch = (Mm3Character *)(DG + 0xB9D6 + i * 0x12F); ch->sp = (int16_t)rnd(100); ch->level = 1 + rnd(20); }
	a[1] = rnd(77);
}
static void setup_movemon(unsigned iter, uint16_t *a) {
	(void)iter; randomize_party();
	for (int i = 0; i < 1024; i++) DG[0x8EF4 + i] = rnd(4);
	for (int m = 0; m < 8; m++) { wr16(DG, 0xAD70 + m * 2, 1 + rnd(28)); wr16(DG, 0xAEC4 + m * 2, 1 + rnd(28)); wr16(DG, 0xB6BC + m * 2, rnd(80)); wr16(DG, 0xB810 + m * 2, rnd(2)); }
	for (int i = 0; i < 100; i++) DG[0x1B20 + i] = rnd(4);
	DG[0x15C] = rnd(2); DG[0x14D] = rnd(2);
	a[0] = (uint16_t)((int)rnd(3) - 1); a[1] = (uint16_t)((int)rnd(3) - 1); a[2] = rnd(8);
}
static void setup_damage(unsigned iter, uint16_t *a) { (void)iter; randomize_conditions(); a[0] = 0xB9D6 + rnd(6) * 0x12F; a[1] = rnd(80); }

typedef struct { const char *name; void (*host)(Cpu *); int nargs, ret; void (*setup)(unsigned, uint16_t *); } DiffCase;
static const DiffCase cases[] = {
	{ "getMonsterResistance", host_getMonsterResistance, 1, 1, setup_resist },
	{ "Spells_subSpellCost", host_Spells_subSpellCost, 2, 1, setup_spellcost },
	{ "moveMonsterBy", host_moveMonsterBy, 3, 0, setup_movemon },
	{ "itemPrice", host_itemPrice, 4, 2, setup_price },
	{ "checkClasses", host_checkClasses, 2, 0, setup_stats },
	{ "rollAttributes", host_rollAttributes, 2, 0, setup_stats },
	{ "getThievery", host_getThievery, 1, 1, setup_thief },
	{ "getWeaponDamage", host_getWeaponDamage, 2, 0, setup_combat },
	{ "hitMonster", host_hitMonster, 2, 1, setup_combat },
	{ "charSavingThrow", host_charSavingThrow, 2, 1, setup_save },
	{ "worstCondition", host_worstCondition, 1, 1, setup_char_cond },
	{ "checkPartyDead", host_checkPartyDead, 0, 0, setup_conditions },
	{ "allHaveGone", host_allHaveGone, 0, 1, setup_conditions },
	{ "charsCantAct", host_charsCantAct, 0, 1, setup_conditions },
	{ "subtractHitPoints", host_subtractHitPoints, 2, 0, setup_damage },
	{ "mazeNeighbourSlot", host_mazeNeighbourSlot, 1, 1, setup_nb },
	{ "mazeGetWordRel", host_mazeGetWordRel, 3, 1, setup_xy },
	{ "mazeGetWordWrap", host_mazeGetWordWrap, 3, 1, setup_xy },
	{ "mazeGetFlagsRel", host_mazeGetFlagsRel, 3, 1, setup_xy },
	{ "mazeSetBits", host_mazeSetBits, 4, 0, setup_xy_in },
	{ "markCellVisited", host_markCellVisited, 2, 0, setup_visit },
	{ "isCellVisited", host_isCellVisited, 2, 1, setup_visit },
	{ "setBit", host_setBit, 3, 0, setup_bit },
	{ "isBitSet", host_isBitSet, 2, 1, setup_bit },
	{ "getCurrentExperience", host_getCurrentExperience, 1, 2, setup_char },
	{ "nextExperienceLevel", host_nextExperienceLevel, 1, 2, setup_char },
	{ "experienceToNextLevel", host_experienceToNextLevel, 1, 2, setup_char },
	{ "giveExperience", host_giveExperience, 2, 0, setup_xp },
};

static void run_ref(const char *name, const uint16_t *args, int nargs, Cpu *c) {
	const RecompEntry *e;
	for (e = recomp_entries_ref_gen; e->name && strcmp(e->name, name); e++) {}
	if (!e->name) { fprintf(stderr, "difftest: no translated reference for %s (run make regen_ref)\n", name); exit(2); }
	memset(c, 0, sizeof *c);
	c->sp = STACK_TOP; c->ds = DSEG;
	for (int i = nargs - 1; i >= 0; i--) PUSH(c, args[i]);
	PUSH(c, 0); PUSH(c, 0);
	e->fn(c);
}

static void run_host(void (*host)(Cpu *), const uint16_t *args, int nargs, Cpu *c) {
	memset(c, 0, sizeof *c);
	c->sp = STACK_TOP; c->ds = DSEG;
	for (int i = nargs - 1; i >= 0; i--) PUSH(c, args[i]);
	host(c);
}

int game_difftest(const char *only, unsigned n) {
	static uint8_t snapshot[65536], mutated[65536], after_ref[65536];
	int total_bad = 0;
	memcpy(snapshot, DG, 65536);
	for (unsigned k = 0; k < sizeof cases / sizeof *cases; k++) {
		const DiffCase *t = &cases[k];
		if (only && strcmp(only, t->name)) continue;
		unsigned bad = 0;
		for (unsigned i = 0; i < n; i++) {
			uint16_t args[8] = { 0 };
			Cpu a, b;
			rs = 1000 + i * 7919 + k;
			memcpy(DG, snapshot, 65536);
			t->setup(i, args);
			memcpy(mutated, DG, 65536);
			mm3_rng_seed(5000 + i);
			run_ref(t->name, args, t->nargs, &a);
			memcpy(after_ref, DG, 65536);
			memcpy(DG, mutated, 65536);
			mm3_rng_seed(5000 + i);
			run_host(t->host, args, t->nargs, &b);
			int same = !t->ret || (a.ax == b.ax && (t->ret < 2 || a.dx == b.dx)); /* ret: 0 none, 1 ax, 2 dx:ax */
			size_t diff_at = 0;
			for (size_t o = 0; same && o < 0xF000; o++) if (after_ref[o] != DG[o]) { same = 0; diff_at = o; }
			if (!same && bad++ < 5)
				printf("MISMATCH %s case %u: translated ax=%04X dx=%04X, readable ax=%04X dx=%04X, first data difference at %04zX\n", t->name, i, a.ax, a.dx, b.ax, b.dx, diff_at);
		}
		printf("%-24s %u cases, %u mismatches\n", t->name, n, bad);
		total_bad += bad;
	}
	return total_bad != 0;
}

/* Shadow mode: during normal play every call of a readable replacement is repeated by the translated original on the same data, and any
 * difference in the results or the data segment is reported once per function.  (Slow: two 64 KB copies per call.) */
void game_shadow(const char *name, void (*host)(Cpu *), Cpu *c, int nargs, int ret) {
	static int enabled = -1;
	if (enabled < 0) enabled = getenv("MM3_SHADOW") != NULL;
	if (!enabled) { host(c); return; }
	static uint8_t before[65536], after_host[65536];
	uint16_t args[8];
	for (int i = 0; i < nargs && i < 8; i++) args[i] = host_arg(c, i);
	uint16_t sp = c->sp;
	memcpy(before, DG, 65536);
	uint32_t rng_before = mm3_rng_get();
	host(c);
	uint32_t rng_after = mm3_rng_get();
	Cpu host_cpu = *c;
	memcpy(after_host, DG, 65536);
	memcpy(DG, before, 65536);
	Cpu ref;
	mm3_rng_seed(rng_before);       /* the original must see the same random numbers */
	run_ref(name, args, nargs, &ref);
	int rng_same = mm3_rng_get() == rng_after;
	mm3_rng_seed(rng_after);
	size_t diff_at = 0;
	int same = rng_same && (!ret || (ref.ax == host_cpu.ax && (ret < 2 || ref.dx == host_cpu.dx)));
	for (size_t o = 0; same && o < 0xF000; o++) if (DG[o] != after_host[o]) { same = 0; diff_at = o; }
	if (!same) {
		static char reported[64][32]; static int nrep;
		int seen = 0;
		for (int i = 0; i < nrep; i++) if (!strcmp(reported[i], name)) seen = 1;
		if (!seen && nrep < 64) {
			snprintf(reported[nrep++], 32, "%s", name);
			fprintf(stderr, "SHADOW MISMATCH %s(", name);
			for (int i = 0; i < nargs; i++) fprintf(stderr, "%04X%s", args[i], i + 1 < nargs ? "," : "");
			fprintf(stderr, "): translated ax=%04X dx=%04X, readable ax=%04X dx=%04X, data differs at %04zX\n", ref.ax, ref.dx, host_cpu.ax, host_cpu.dx, diff_at);
		}
	}
	memcpy(DG, after_host, 65536);   /* continue with the readable version's result */
	*c = host_cpu; c->sp = sp;
}
