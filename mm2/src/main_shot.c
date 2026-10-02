/* Renders one first-person view (with a status line) to a PNG without SDL.
 *   mm2_shot MAP X Y N|E|S|W out.png [game dir]            */
#include "mm2_map.h"
#include "mm2_png.h"
#include "mm2_text.h"
#include "mm2_fight.h"
#include "mm2_monpic.h"
#include "mm2_ui.h"
#include "mm2_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int shot_state = 7;
static int shot_rng(void *ud, int lo, int hi) {
	(void)ud;
	shot_state = shot_state * 1103515245 + 12345;
	return hi <= lo ? lo : lo + (int)(((unsigned)shot_state >> 8) % (unsigned)(hi - lo + 1));
}

int main(int argc, char **argv) {
	Mm2Game g;
	Mm2View v;
	Mm2Font font;
	uint8_t map[512], attr[64], canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	char line[48];
	int m, x, y, style;
	if (argc >= 4 && strcmp(argv[1], "inn") == 0) {   /* mm2_shot inn TOWN out.png [add ids...] */
		static Mm2Roster roster;
		uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
		int i;
		mm2_game_init(&g, NULL);
		if (!mm2_load_roster(&g, &roster) || !mm2_font_load(&g, &font)) return 1;
		for (i = 4; i < argc; i++)
			mm2_inn_add(&roster, atoi(argv[i]));
		mm2_ui_draw_inn(canvas, &font, &roster, atoi(argv[2]));
		return mm2_write_png(argv[3], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
	}
	if (argc >= 4 && (strcmp(argv[1], "sheet") == 0 || strcmp(argv[1], "train") == 0)) {   /* mm2_shot sheet|train ID out.png */
		static Mm2Roster roster;
		static Mm2Item items[MM2_ITEMS];
		uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
		int i;
		mm2_game_init(&g, NULL);
		if (!mm2_load_roster(&g, &roster) || !mm2_font_load(&g, &font) || !mm2_load_items(&g, items)) return 1;
		if (argv[1][0] == 's') {
			mm2_ui_draw_sheet(canvas, &font, &roster.chars[atoi(argv[2])], items);
		} else {
			for (i = 0; i < 6; i++) mm2_inn_add(&roster, i);
			mm2_ui_draw_training(canvas, &font, &roster, atoi(argv[2]), NULL);
		}
		return mm2_write_png(argv[3], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
	}
	if (argc >= 5 && strcmp(argv[1], "shop") == 0) {   /* mm2_shot shop temple|guild|smith TOWN out.png */
		static Mm2Roster roster;
		static Mm2Item items[MM2_ITEMS];
		uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
		int i, town = atoi(argv[3]);
		mm2_game_init(&g, NULL);
		if (!mm2_load_roster(&g, &roster) || !mm2_font_load(&g, &font) || !mm2_load_items(&g, items)) return 1;
		for (i = 0; i < 6; i++) mm2_inn_add(&roster, i);
		if (argv[2][0] == 's') mm2_ui_draw_smith(canvas, &font, &roster, town, 0, 1, 1, items, NULL);
		else mm2_ui_draw_temple(canvas, &font, &roster, town, 3, argv[2][0] == 'g', NULL);
		return mm2_write_png(argv[4], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
	}
	if (argc >= 5 && strcmp(argv[1], "tavern") == 0) {   /* mm2_shot tavern TOWN SUBMENU out.png */
		static Mm2Roster roster;
		static Mm2BuildingText text;
		uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
		int i;
		mm2_game_init(&g, NULL);
		if (!mm2_load_roster(&g, &roster) || !mm2_font_load(&g, &font) || !mm2_btext_load(&g, 1, &text)) return 1;
		for (i = 0; i < 3; i++) mm2_inn_add(&roster, i);
		mm2_ui_draw_tavern(canvas, &font, &roster, atoi(argv[2]), 0, atoi(argv[3]), &text, NULL);
		return mm2_write_png(argv[4], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
	}
	if (argc >= 3 && strcmp(argv[1], "battle") == 0) {   /* mm2_shot battle out.png [rounds] */
		static Mm2Roster roster;
		static Mm2Item items[MM2_ITEMS];
		static Mm2Monster table[MM2_MONSTERS];
		static Mm2Fight f;
		static const uint8_t ids[6] = {1, 2, 3, 3, 5, 7};
		uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H], pic[96 * 96];
		Mm2Rng rng = {shot_rng, NULL};
		Mm2MonPic mp;
		int i, idx, steps = argc > 3 ? atoi(argv[3]) : 4;
		mm2_game_init(&g, NULL);
		if (!mm2_load_roster(&g, &roster) || !mm2_font_load(&g, &font) || !mm2_load_items(&g, items) || !mm2_load_monsters(&g, table)) return 1;
		for (i = 0; i < 6; i++) mm2_inn_add(&roster, i);
		mm2_fight_start(&f, &roster, table, items, ids, 6, MM2_SURPRISE_NONE, &rng);
		for (i = 0; i < steps && mm2_fight_next(&f, &idx) == MM2_ACTOR_PARTY; i++)
			mm2_fight_party_attack(&f, idx, 0, 0);
		mm2_fight_next(&f, &idx);
		memset(pic, 0, sizeof(pic));
		if (mm2_monpic_load(&g, table[f.b.id[0]].picture, 0, &mp)) { mm2_monpic_frame(&mp, 0, 0, pic); mm2_monpic_free(&mp); }
		mm2_ui_draw_battle(canvas, &font, &f, pic, idx, "A-Attack S-Shoot C-Cast B-Block R-Run");
		return mm2_write_png(argv[2], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
	}
	if (argc < 6) {
		fprintf(stderr, "usage: %s MAP X Y N|E|S|W out.png [game dir]\n", argv[0]);
		return 2;
	}
	m = atoi(argv[1]); x = atoi(argv[2]); y = atoi(argv[3]);
	mm2_game_init(&g, argc > 6 ? argv[6] : NULL);
	if (!mm2_load_map(&g, m, map) || !mm2_font_load(&g, &font)) {
		fprintf(stderr, "cannot load map/font\n");
		return 1;
	}
	style = mm2_map_style(m);
	if (mm2_style_is_outdoor(style)) {
		const char *sp = mm2_load_attrib(&g, m, attr) ? mm2_special_bank(attr[4]) : "OCEAN";
		if (!mm2_view_load_outdoor(&v, &g, sp)) return 1;
		mm2_view_render_outdoor(&v, canvas, map, x, y, argv[4][0]);
	} else {
		if (!mm2_view_load_indoor(&v, &g, mm2_style_name(style))) return 1;
		mm2_view_render_indoor(&v, canvas, map, x, y, argv[4][0]);
	}
	snprintf(line, sizeof(line), "Map %d  x=%d y=%d facing %c", m, x, y, argv[4][0]);
	mm2_draw_text(canvas, &font, 1, 17, line, 15, -1);
	mm2_view_free(&v);
	return mm2_write_png(argv[5], canvas, MM2_SCREEN_W, MM2_SCREEN_H, MM2_EGA_PALETTE, 16) ? 0 : 1;
}
