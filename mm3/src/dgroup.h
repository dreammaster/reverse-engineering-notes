/* The game's initialised data segment (DGROUP) dumped by tools/mm3_dgroup.py: lookup tables are read at run time by the
 * DGROUP offsets used in docs/ and names/mm3.tsv, so no game data is compiled into the program. */
#ifndef MM3_DGROUP_H
#define MM3_DGROUP_H

#include <stddef.h>
#include <stdint.h>

#define MM3_DGROUP_SIZE 0x10000

typedef struct {
	uint8_t *data; /* MM3_DGROUP_SIZE bytes */
} Mm3Dgroup;

/* DGROUP offsets of tables (names from names/mm3.tsv, linear address - 286F0h). */
enum {
	MM3_DG_MON_ANIM_FRAMES = 0x1A6C, /* byte per monster id: animation range (Map_load: rnd(0, value)) */
	MM3_DG_MAP_MONSTER_PICS = 0x2752, /* 3 bytes per map, map 1 first */
	MM3_DG_OBJECT_ANIM_FRAMES = 0x2815 /* byte per object picture slot id */
};

int mm3_dgroup_load(Mm3Dgroup *dg, const char *path);
void mm3_dgroup_free(Mm3Dgroup *dg);
static inline uint8_t mm3_dg_u8(const Mm3Dgroup *dg, unsigned off) { return dg->data[off & 0xFFFF]; }
static inline uint16_t mm3_dg_u16(const Mm3Dgroup *dg, unsigned off) { return (uint16_t)(dg->data[off & 0xFFFF] | (dg->data[(off + 1) & 0xFFFF] << 8)); }

#endif
