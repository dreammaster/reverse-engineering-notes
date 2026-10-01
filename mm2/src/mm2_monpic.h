/* Monster pictures (MONSTERS.16 / MONSTERS.4; docs/file-formats.md).  Port of tools/mm2_monsters.py. */
#ifndef MM2_MONPIC_HEADER
#define MM2_MONPIC_HEADER

#include "mm2_files.h"

#define MM2_MONPIC_W 96
#define MM2_MONPIC_H 96
#define MM2_MONPIC_COUNT 75

typedef struct {
	Mm2Blob bank;   /* decompressed bank of one picture id */
	int frames;
	int cga;
} Mm2MonPic;

/* Loads picture `id` (0-74) from MONSTERS.16 (cga = 0) or MONSTERS.4 (cga = 1).  Returns 0 if the id is unused. */
int mm2_monpic_load(const Mm2Game *g, int id, int cga, Mm2MonPic *p);
void mm2_monpic_free(Mm2MonPic *p);
/* Renders frame `k` (0 = base) into out[96*96]; the base is filled with `background` first. */
void mm2_monpic_frame(const Mm2MonPic *p, int k, uint8_t background, uint8_t *out);

#endif
