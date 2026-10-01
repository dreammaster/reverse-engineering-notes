#include "mm2_ui.h"
#include "mm2_gfx.h"

#include <stdio.h>
#include <string.h>

static const char *const CLASS_NAME[8] = {"Knight", "Paladin", "Archer", "Cleric", "Sorcerer", "Robber", "Ninja", "Barbarian"};
static const char *const RACE_NAME[5] = {"Human", "Elf", "Dwarf", "Gnome", "H-Orc"};

void mm2_ui_draw_inn(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town) {
	int list[MM2_ROSTER_CHARS], n, i, row = 2;
	char line[48];
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	mm2_draw_text(canvas, font, 14, 0, " Inn ", 14, -1);
	n = mm2_inn_list(roster, town, list);
	for (i = 0; i < n && i < 24; i++) {
		const Mm2Char *c = &roster->chars[list[i]];
		int inParty = 0, k;
		for (k = 0; k < mm2_party_size(roster); k++)
			if (mm2_party_member(roster, k) == list[i]) inParty = 1;
		snprintf(line, sizeof(line), "%c) %-11.11s %-8s %-6s L%-2d %s", 'A' + i, (const char *)c->raw,
				 CLASS_NAME[c->raw[MC_CLASS] & 7], RACE_NAME[c->raw[MC_RACE] % 5], c->raw[MC_LEVEL], inParty ? "*" : "");
		mm2_draw_text(canvas, font, 0, row++, line, inParty ? 10 : 15, -1);
	}
	if (!n) mm2_draw_text(canvas, font, 0, row, "Nobody lives in this town.", 7, -1);
	snprintf(line, sizeof(line), "Party: %d (max 6 + 2 hirelings)", mm2_party_size(roster));
	mm2_draw_text(canvas, font, 0, 22, line, 11, -1);
	mm2_draw_text(canvas, font, 0, 23, "A-X add/remove   Z or Esc: leave", 7, -1);
}

