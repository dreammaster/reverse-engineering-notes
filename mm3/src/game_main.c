/* mm3game DATADIR [--headless] [--keys HEX,...] [--shot out.bmp]
 * The recompiled game: translated routines (gen/game_gen.c) over the original program image, with the host layer of game_*.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

int video_init(int headless_mode);
void video_save_bmp(const char *path);
void video_set_key_script(const unsigned *keys, int n);
void video_set_shot(const char *path);

void call_loadMonsterData(Cpu *c);
void call_loadSavedGame(Cpu *c);
void call_exploreLoop(Cpu *c);
void call_introSequence(Cpu *c);
void call_rosterMenu(Cpu *c);
void call_getFiles(Cpu *c);

int main(int argc, char **argv) {
	Cpu c;
	int headless = 0, intro = 0, at_n = 0, roster_menu = 1, difftest = 0;
	const char *difftest_only = NULL;
	unsigned difftest_n = 500;
	const char *cur_path = NULL;
	unsigned at[4];
	const char *shot = NULL;
	unsigned keys[256];
	int nkeys = 0;
	if (argc < 2) { fprintf(stderr, "usage: %s DATADIR [--headless] [--keys HEX,HEX..] [--shot out.bmp]\n", argv[0]); return 2; }
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) headless = 1;
		else if (!strcmp(argv[i], "--intro")) intro = 1;
		else if (!strcmp(argv[i], "--at") && i + 1 < argc) { at_n = sscanf(argv[++i], "%u,%u,%u,%u", &at[0], &at[1], &at[2], &at[3]); }
		else if (!strcmp(argv[i], "--bare")) roster_menu = 0; /* skip rosterMenu: straight into exploreLoop without the full HUD */
		else if (!strcmp(argv[i], "--cur") && i + 1 < argc) cur_path = argv[++i]; /* start from this saved game (a .MM3 file) instead of MM3.CUR */
		else if (!strcmp(argv[i], "--difftest")) { difftest = 1; if (i + 1 < argc && argv[i + 1][0] != '-') difftest_only = argv[++i]; if (i + 1 < argc && argv[i + 1][0] != '-') difftest_n = (unsigned)atoi(argv[++i]); }
		else if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
		else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			for (char *t = strtok(argv[++i], ","); t && nkeys < 256; t = strtok(NULL, ",")) { unsigned mx, my; keys[nkeys++] = (t[0] == 'w') ? 0x40000000u | (unsigned)atoi(t + 1) : (t[0] == 'm' && sscanf(t + 1, "%u:%u", &mx, &my) == 2) ? 0x80000000u | (mx << 12) | my : (unsigned)strtoul(t, NULL, 16); } /* mX:Y = click at pixel X,Y */
		}
	}
	if (game_init(argv[1]) || video_init(headless)) return 1;
	if (cur_path) { Mm3Cc fresh; if (mm3_cc_open(&fresh, cur_path)) { fprintf(stderr, "cannot open %s\n", cur_path); return 1; } mm3_cc_close(&G.cur); G.cur = fresh; }
	sound_init(headless);
	video_set_key_script(keys, nkeys);
	video_set_shot(shot);
	memset(&c, 0, sizeof c);
	c.sp = STACK_TOP; c.ds = DSEG;

	/* the parts of _main this bring-up needs */
	{
		uint16_t roster = dos_alloc(0x2382 + 16);
		wr16(DG, 0xECA8, 0); wr16(DG, 0xECAA, roster); /* Roster_buffer (far pointer) */
	}
	{ /* _main allocates the buffer the song files are loaded into (word_332DC/E: far pointer) */
		uint16_t seg = dos_alloc(0x1890);
		wr16(DG, 0xABEC, 0); wr16(DG, 0xABEE, seg);
	}
	DG[0xE8F8] = DG[0xE8F9] = 1; /* Option_sfx, Option_music (set by _main before the intro) */
	if (intro) game_call(call_introSequence, &c, NULL, 0);
	game_call(call_loadMonsterData, &c, NULL, 0);
	game_call(call_loadSavedGame, &c, NULL, 0);
	{ /* the party members are copies of roster characters (their indices are in the party block) */
		uint8_t *roster = SEGP(rd16(DG, 0xECAA));
		unsigned count = DG[0xE8EA];
		for (unsigned i = 0; i < count && i < 8; i++) {
			unsigned idx = DG[0xE8EA + 1 + i];
			if (idx != 0xFF) memcpy(DG + 0xB9D6 + i * 0x12F, roster + idx * 0x12F, 0x12F);
		}
	}
	{ /* the startup code lists the saved games (*.mm3) and picks one: byte_29116 is its index in the 13-byte name table at E836h (0FFh = none: rosterMenu ends) */
		uint16_t pattern[2] = { 0x3433, DSEG };
		game_call(call_getFiles, &c, pattern, 2);
		fprintf(stderr, "getFiles: count %u first [%s]\n", DG[0xA25], (char *)DG + 0xE836);
		DG[0xA26] = DG[0xE836] ? 0 : 0xFF;
	}
	if (roster_menu) game_call(call_rosterMenu, &c, NULL, 0);
	if (at_n == 4) { DG[0xE8F7] = at[0]; DG[0xE8F5] = at[1]; DG[0xE8F6] = at[2]; DG[0xE8F4] = at[3]; } /* --at MAP,X,Y,FACING */
	if (difftest) return game_difftest(difftest_only, difftest_n);
	game_call(call_exploreLoop, &c, NULL, 0);
	if (shot) video_save_bmp(shot);
	return 0;
}
