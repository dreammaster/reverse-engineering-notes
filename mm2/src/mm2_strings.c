/* TODO(review): block starts come from the table at DGROUP:52F4 (resident load_building_text, mm2.asm IDA 0x1670A..0x167B8).  Only
 * the tavern layout (building 1, ovl/2BRAIN.asm tavern_menu IDA 0x1D15A: 5x4, 6, 14, 5x8, 5x8, 6, 5x6 strings) was decoded and
 * checked by reading the text; building ids for the other places (blacksmith 2, temple 3 ...) and their layouts were not. */
#include "mm2_strings.h"

#include <string.h>

/* DGROUP:52F4 */
static const uint16_t BLOCK_START[8] = {0, 1596, 3932, 4742, 6212, 20306, 21587, 21061};

int mm2_btext_load(const Mm2Game *g, int building, Mm2BuildingText *t) {
	Mm2Blob raw = mm2_load_lzw_file(g, "STR.DAT");
	int i, start = building >= 0 && building < 8 ? BLOCK_START[building] : -1, n = 0, at = 0;
	memset(t, 0, sizeof(*t));
	if (!raw.data || start < 0) {
		mm2_blob_free(&raw);
		return 0;
	}
	for (i = 0; i < MM2_BTEXT_SIZE && (size_t)(start + i) < raw.size; i++) {
		int c = (raw.data[start + i] + 0x1C) & 0xFF;
		t->buf[i] = c == 0x1D ? 0 : (char)c;
	}
	mm2_blob_free(&raw);
	while (at < MM2_BTEXT_SIZE && n < MM2_BTEXT_MAX_STRINGS) {
		t->off[n++] = at;
		while (at < MM2_BTEXT_SIZE && t->buf[at])
			at++;
		at++;
	}
	t->count = n;
	return 1;
}

const char *mm2_btext_str(const Mm2BuildingText *t, int index) {
	return index >= 0 && index < t->count ? t->buf + t->off[index] : "";
}

void mm2_tavern_text_layout(Mm2TavernText *t) {
	int n = 0, i, k;
	for (i = 0; i < 5; i++)
		for (k = 0; k < 4; k++) t->town[i][k] = n++;
	for (k = 0; k < 6; k++) t->menu[k] = n++;
	for (k = 0; k < 14; k++) t->message[k] = n++;
	for (i = 0; i < 5; i++)
		for (k = 0; k < 8; k++) t->rumourE[i][k] = n++;
	for (i = 0; i < 5; i++)
		for (k = 0; k < 8; k++) t->rumourD[i][k] = n++;
	for (k = 0; k < 6; k++) t->drink[k] = n++;
	for (i = 0; i < 5; i++)
		for (k = 0; k < 6; k++) t->special[i][k] = n++;
}
