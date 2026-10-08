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

#define H(name) void host_##name(Cpu *c)
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

typedef struct { const char *name; void (*host)(Cpu *); int nargs, ret; void (*setup)(unsigned, uint16_t *); } DiffCase;
static const DiffCase cases[] = {
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
