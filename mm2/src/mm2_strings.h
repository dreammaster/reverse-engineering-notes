/* STR.DAT building texts (resident load_building_text 0x1670A / str_next 0x167BC; docs/file-formats.md):
 * the decompressed file is read in blocks of 960h bytes starting at DGROUP:52F4[building]; every byte is
 * (b + 1Ch) mod 256 and 1Dh becomes a NUL, so a block is a sequence of NUL-terminated strings that each building
 * consumes in a fixed order. */
#ifndef MM2_STRINGS_H
#define MM2_STRINGS_H

#include "mm2_files.h"

#define MM2_BTEXT_SIZE 0x960
#define MM2_BTEXT_MAX_STRINGS 512

typedef struct {
	char buf[MM2_BTEXT_SIZE + 1];
	int off[MM2_BTEXT_MAX_STRINGS];
	int count;
} Mm2BuildingText;

/* building: 1 tavern, 2 blacksmith, ... (the argument of load_building_text).  Returns 0 on failure. */
int mm2_btext_load(const Mm2Game *g, int building, Mm2BuildingText *t);
const char *mm2_btext_str(const Mm2BuildingText *t, int index);

/* The tavern's text block layout (ovl/2BRAIN.asm tavern_menu, 0x1D15A): strings are consumed in this order. */
typedef struct {
	int town[5][4];        /* DGROUP:573E: 4 strings per town (indices into the block) */
	int menu[6];           /* 5766 */
	int message[14];       /* 5722 */
	int rumourE[5][8];     /* 56D2: per town 4 rumours of 2 lines */
	int rumourD[5][8];     /* 5676 */
	int drink[6];          /* 56C6 */
	int special[5][6];     /* 577E: per town 3 specialties of 2 lines */
} Mm2TavernText;

void mm2_tavern_text_layout(Mm2TavernText *t);

#endif
