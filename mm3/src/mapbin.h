/* MAZEnn.BIN: the monsters and objects of a map (decoded from Map_load, 43698; docs/data-files.md). */
#ifndef MM3_MAPBIN_H
#define MM3_MAPBIN_H

#include <stddef.h>
#include <stdint.h>

#include "dgroup.h"
#include "party.h"

#define MM3_MAX_MONSTERS 0xAA /* 170 */
#define MM3_MAX_OBJECTS 0x50  /* 80 */
#define MM3_PIC_UNUSED 0x2A   /* picture slot value: not used */

typedef struct {
	uint8_t x, y;
	uint8_t id;      /* monster id (index into the MON*.DAT columns) */
	uint8_t pic_sel; /* b & 3: which of the map's three monster pictures */
	uint8_t anim_range; /* the engine starts the animation phase at rnd(0, anim_range) */
} Mm3MapMonster;

typedef struct {
	uint8_t x, y;
	uint8_t slot;     /* index into the map's five object picture slots */
	uint8_t pic;      /* picture id = slots[slot], index of the name table at DGROUP 58D4h */
	uint8_t anim_range;
} Mm3MapObject;

typedef struct {
	Mm3MapMonster monsters[MM3_MAX_MONSTERS];
	unsigned monster_count;
	uint8_t pic_slots[5]; /* object picture ids of the map (3Eh/2Ah rules below applied) */
	Mm3MapObject objects[MM3_MAX_OBJECTS];
	unsigned object_count;
	int monsters_overflow, objects_overflow; /* the game shows "Max ... Exceeded" and exits */
} Mm3MapBin;

/* `map` is the map id as in MAZE.PTY / Teleport (file MAZE%02d.BIN); Map_load itself receives map - 1.
 * `party` supplies the game flags that swap three maps' picture slots (maps 42, 48, 57: flags 41h, 4Eh, 5Bh); may be NULL. */
int mm3_mapbin_load(Mm3MapBin *m, const uint8_t *data, size_t len, unsigned map, const Mm3Dgroup *dg, const Mm3Party *party);

#endif
