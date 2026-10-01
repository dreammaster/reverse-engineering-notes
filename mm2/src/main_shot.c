/* Renders one first-person view (with a status line) to a PNG without SDL.
 *   mm2_shot MAP X Y N|E|S|W out.png [game dir]            */
#include "mm2_map.h"
#include "mm2_png.h"
#include "mm2_text.h"
#include "mm2_ui.h"
#include "mm2_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
