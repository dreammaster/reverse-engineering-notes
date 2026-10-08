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

int main(int argc, char **argv) {
	Cpu c;
	int headless = 0;
	const char *shot = NULL;
	unsigned keys[256];
	int nkeys = 0;
	if (argc < 2) { fprintf(stderr, "usage: %s DATADIR [--headless] [--keys HEX,HEX..] [--shot out.bmp]\n", argv[0]); return 2; }
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--headless")) headless = 1;
		else if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
		else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
			for (char *t = strtok(argv[++i], ","); t && nkeys < 256; t = strtok(NULL, ",")) keys[nkeys++] = (unsigned)strtoul(t, NULL, 16);
		}
	}
	if (game_init(argv[1]) || video_init(headless)) return 1;
	video_set_key_script(keys, nkeys);
	video_set_shot(shot);
	memset(&c, 0, sizeof c);
	c.sp = STACK_TOP; c.ds = DSEG;

	/* the parts of _main this bring-up needs */
	{
		uint16_t roster = dos_alloc(0x2382 + 16);
		wr16(DG, 0xECA8, 0); wr16(DG, 0xECAA, roster); /* Roster_buffer (far pointer) */
	}
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
	game_call(call_exploreLoop, &c, NULL, 0);
	if (shot) video_save_bmp(shot);
	return 0;
}
