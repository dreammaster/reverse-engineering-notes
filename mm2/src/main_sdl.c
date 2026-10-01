/* SDL2 explorer: walk through the maps and run their event scripts.
 *   arrows: move / turn   PgUp/PgDn: previous/next map   Y / N: answer for yes/no prompts   Esc: quit
 * Messages from event scripts are shown under the view; teleports change the map.
 * Usage: mm2 [game dir]   (default: $MM2_DIR or the GOG install path) */
#include "mm2_game.h"
#include "mm2_inn.h"
#include "mm2_map.h"
#include "mm2_text.h"
#include "mm2_ui.h"
#include "mm2_view.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define SCALE 3

typedef struct {
	Mm2Roster roster;
	int inInn, innTown;
	int inTrain, inSheet, sheetSlot;
	Mm2Item items[MM2_ITEMS];
	Mm2GameSession s;
	Mm2View view;
	int viewMap, viewLoaded, outdoors;
} App;

/* (Re)loads the view graphics when the map changed. */
static int sync_view(App *a, const Mm2Game *g) {
	uint8_t attr[64];
	int style;
	if (a->viewLoaded && a->viewMap == a->s.map) return 1;
	style = mm2_map_style(a->s.map);
	if (a->viewLoaded) mm2_view_free(&a->view);
	a->viewLoaded = 0;
	a->outdoors = mm2_style_is_outdoor(style);
	if (a->outdoors) {
		const char *special = "OCEAN";
		if (mm2_load_attrib(g, a->s.map, attr)) special = mm2_special_bank(attr[4]);
		if (!mm2_view_load_outdoor(&a->view, g, special)) return 0;
	} else if (!mm2_view_load_indoor(&a->view, g, mm2_style_name(style))) {
		return 0;
	}
	a->viewLoaded = 1;
	a->viewMap = a->s.map;
	return 1;
}

static int start_map(App *a, const Mm2Game *g, int map, int x, int y, char facing) {
	int yes = a->s.yesNo ? a->s.yesNo : 1;
	if (a->s.files) mm2_session_end(&a->s);
	if (!mm2_session_start(&a->s, g, map, x, y, facing)) return 0;
	a->s.yesNo = yes;
	return sync_view(a, g);
}

static void draw_messages(uint8_t *canvas, const Mm2Font *font, const Mm2GameSession *s) {
	int row = 19, i;
	for (i = 0; i < s->nMessages && row < 25; i++) {
		const char *p = s->messages[i].text;
		char line[41];
		while (*p && row < 25) {
			int n = 0;
			while (*p && *p != '\n' && n < 40)
				line[n++] = *p++;
			line[n] = 0;
			if (*p == '\n') p++;
			mm2_draw_text(canvas, font, 0, row++, line, 14, -1);
		}
	}
	for (i = 0; i < s->nLocations && row < 25; i++) {
		char line[41];
		snprintf(line, sizeof(line), "[%s - not implemented yet]", mm2_location_name(s->locations[i]));
		mm2_draw_text(canvas, font, 0, row++, line, 11, -1);
	}
	if (s->fightRequested && row < 25) mm2_draw_text(canvas, font, 0, row, "[a fight starts here]", 12, -1);
}

int main(int argc, char **argv) {
	Mm2Game g;
	App a;
	Mm2Font font;
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Texture *tex;
	uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	uint32_t pixels[MM2_SCREEN_W * MM2_SCREEN_H];
	int running = 1, dirty = 1, i;
	const char *innMsg = NULL;
	char trainMsg[64] = "";

	mm2_game_init(&g, argc > 1 ? argv[1] : NULL);
	if (!mm2_font_load(&g, &font)) {
		fprintf(stderr, "cannot load MM2.CH\n");
		return 1;
	}
	memset(&a, 0, sizeof(a));
	mm2_load_items(&g, a.items);
	if (!mm2_load_roster(&g, &a.roster)) fprintf(stderr, "warning: cannot load ROSTER.DAT\n");
	if (!start_map(&a, &g, 0, 8, 8, 'N')) {
		fprintf(stderr, "cannot load game data from %s\n", g.dir);
		return 1;
	}
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	win = SDL_CreateWindow("Might and Magic II", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
						   MM2_SCREEN_W * SCALE, MM2_SCREEN_H * SCALE, 0);
	ren = SDL_CreateRenderer(win, -1, 0);
	tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MM2_SCREEN_W, MM2_SCREEN_H);

	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) running = 0;
			if (e.type != SDL_KEYDOWN) continue;
			if (a.inInn) {
				int k = e.key.keysym.sym;
				if (k == SDLK_ESCAPE || k == SDLK_z) {
					mm2_inn_leave(&a.roster, a.innTown);
					a.inInn = 0;
				} else if (k >= SDLK_a && k <= SDLK_x) {
					int list[MM2_ROSTER_CHARS], n = mm2_inn_list(&a.roster, a.innTown, list), idx = k - SDLK_a;
					if (idx < n) {
						Mm2InnResult r = mm2_inn_add(&a.roster, list[idx]);
						if (r == MM2_INN_ALREADY) mm2_inn_remove(&a.roster, list[idx]);
						else if (r == MM2_INN_FULL) innMsg = "*** Party is Full ***";
					}
				}
				dirty = 1;
				continue;
			}
			if (a.inTrain) {
				int k = e.key.keysym.sym;
				trainMsg[0] = 0;
				if (k == SDLK_ESCAPE) {
					a.inTrain = 0;
				} else if (k >= SDLK_1 && k <= SDLK_8 && k - SDLK_1 < mm2_party_size(&a.roster)) {
					Mm2Char *c = &a.roster.chars[mm2_party_member(&a.roster, k - SDLK_1)];
					Mm2TrainResult r = mm2_train_check(c, a.innTown);
					if (r == MM2_TRAIN_OK) {
						Mm2LevelUp lu = mm2_level_up(c, a.innTown);
						snprintf(trainMsg, sizeof(trainMsg), "You gained %d hit points%s", lu.hpGained, lu.newSpells ? " and new spells" : "");
					} else {
						snprintf(trainMsg, sizeof(trainMsg), "%s", r == MM2_TRAIN_DISABLED ? "Sorry - not in condition." :
								 r == MM2_TRAIN_NEED_EXP ? "Sorry - not enough experience." : "Sorry - you need more gold.");
					}
				}
				dirty = 1;
				continue;
			}
			if (a.inSheet) {
				int k = e.key.keysym.sym;
				if (k >= SDLK_1 && k <= SDLK_8 && k - SDLK_1 < mm2_party_size(&a.roster)) a.sheetSlot = k - SDLK_1;
				else a.inSheet = 0;
				dirty = 1;
				continue;
			}
			a.s.nMessages = a.s.nLocations = 0;
			a.s.fightRequested = 0;
			switch (e.key.keysym.sym) {
			case SDLK_c:
				if (mm2_party_size(&a.roster)) {
					a.inSheet = 1;
					a.sheetSlot = 0;
					dirty = 1;
				}
				break;
			case SDLK_ESCAPE: running = 0; break;
			case SDLK_LEFT: mm2_session_turn(&a.s, -1); dirty = 1; break;
			case SDLK_RIGHT: mm2_session_turn(&a.s, 1); dirty = 1; break;
			case SDLK_UP:
			case SDLK_DOWN:
				if (mm2_session_step(&a.s, e.key.keysym.sym == SDLK_DOWN)) {
					sync_view(&a, &g);   /* a teleport may have changed the map */
					dirty = 1;
					if (a.s.nLocations && a.s.locations[0] == 1 && a.s.map <= 4) {
						a.inInn = 1;
						a.innTown = a.s.map;
					}
					if (a.s.nLocations && a.s.locations[0] == 2 && a.s.map <= 4) {
						a.inTrain = 1;
						a.innTown = a.s.map;
					}
				}
				break;
			case SDLK_y: a.s.yesNo = 1; dirty = 1; break;
			case SDLK_n: a.s.yesNo = 0; dirty = 1; break;
			case SDLK_PAGEUP:
			case SDLK_PAGEDOWN: {
				int m = (a.s.map + (e.key.keysym.sym == SDLK_PAGEUP ? MM2_MAPS - 1 : 1)) % MM2_MAPS;
				if (start_map(&a, &g, m, 8, 8, 'N')) dirty = 1;
				break;
			}
			default: break;
			}
		}
		if (dirty) {
			char line[48];
			if (a.inTrain) {
				mm2_ui_draw_training(canvas, &font, &a.roster, a.innTown, trainMsg[0] ? trainMsg : NULL);
			} else if (a.inSheet) {
				mm2_ui_draw_sheet(canvas, &font, &a.roster.chars[mm2_party_member(&a.roster, a.sheetSlot)], a.items);
				mm2_draw_text(canvas, &font, 0, 24, "1-8: other member   any other key: back", 7, -1);
			} else if (a.inInn) {
				mm2_ui_draw_inn(canvas, &font, &a.roster, a.innTown);
				if (innMsg) mm2_draw_text(canvas, &font, 0, 21, innMsg, 12, -1);
				innMsg = NULL;
			} else if (a.outdoors)
				mm2_view_render_outdoor(&a.view, canvas, a.s.data, a.s.x, a.s.y, a.s.facing);
			else
				mm2_view_render_indoor(&a.view, canvas, a.s.data, a.s.x, a.s.y, a.s.facing);
			if (!a.inInn && !a.inTrain && !a.inSheet) {
				snprintf(line, sizeof(line), "Map %d  x=%d y=%d facing %c  answer:%c", a.s.map, a.s.x, a.s.y, a.s.facing,
						 a.s.yesNo ? 'Y' : 'N');
				mm2_draw_text(canvas, &font, 0, 17, line, 15, -1);
				draw_messages(canvas, &font, &a.s);
			}
			for (i = 0; i < MM2_SCREEN_W * MM2_SCREEN_H; i++)
				pixels[i] = 0xFF000000u | MM2_EGA_PALETTE[canvas[i] & 15];
			SDL_UpdateTexture(tex, NULL, pixels, MM2_SCREEN_W * 4);
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, tex, NULL, NULL);
			SDL_RenderPresent(ren);
			snprintf(line, sizeof(line), "Might and Magic II - map %d (%d,%d) %c", a.s.map, a.s.x, a.s.y, a.s.facing);
			SDL_SetWindowTitle(win, line);
			dirty = 0;
		}
		SDL_Delay(15);
	}
	if (a.viewLoaded) mm2_view_free(&a.view);
	mm2_session_end(&a.s);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
