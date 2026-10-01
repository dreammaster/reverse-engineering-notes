/* SDL2 maze viewer: walk through any map.
 *   arrows: move / turn   PgUp/PgDn: previous/next map   Esc: quit
 * Usage: mm2 [game dir]   (default: $MM2_DIR or the GOG install path) */
#include "mm2_map.h"
#include "mm2_text.h"
#include "mm2_view.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define SCALE 3

typedef struct {
	int map, x, y;
	char facing;
	uint8_t data[512];
	Mm2View view;
	int viewLoaded, outdoors;
} State;

static char turn(char f, int dir) {
	const char order[] = "NESW";
	const char *p = strchr(order, f);
	int i = p ? (int)(p - order) : 0;
	return order[(i + (dir > 0 ? 1 : 3)) & 3];
}

static int load_map(State *s, const Mm2Game *g, int map) {
	uint8_t attr[64];
	int style = mm2_map_style(map);
	if (!mm2_load_map(g, map, s->data)) return 0;
	if (s->viewLoaded) mm2_view_free(&s->view);
	s->viewLoaded = 0;
	s->outdoors = mm2_style_is_outdoor(style);
	if (s->outdoors) {
		const char *special = "OCEAN";
		if (mm2_load_attrib(g, map, attr)) special = mm2_special_bank(attr[4]);
		if (!mm2_view_load_outdoor(&s->view, g, special)) return 0;
	} else if (!mm2_view_load_indoor(&s->view, g, mm2_style_name(style))) {
		return 0;
	}
	s->viewLoaded = 1;
	s->map = map;
	return 1;
}

int main(int argc, char **argv) {
	Mm2Game g;
	State s;
	Mm2Font font;
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Texture *tex;
	uint8_t canvas[MM2_SCREEN_W * MM2_SCREEN_H];
	uint32_t pixels[MM2_SCREEN_W * MM2_SCREEN_H];
	int running = 1, dirty = 1, i;

	mm2_game_init(&g, argc > 1 ? argv[1] : NULL);
	if (!mm2_font_load(&g, &font)) {
		fprintf(stderr, "cannot load MM2.CH
");
		return 1;
	}
	memset(&s, 0, sizeof(s));
	s.x = 8;
	s.y = 8;
	s.facing = 'N';
	if (!load_map(&s, &g, 0)) {
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
			switch (e.key.keysym.sym) {
			case SDLK_ESCAPE: running = 0; break;
			case SDLK_LEFT: s.facing = turn(s.facing, -1); dirty = 1; break;
			case SDLK_RIGHT: s.facing = turn(s.facing, 1); dirty = 1; break;
			case SDLK_UP:
			case SDLK_DOWN: {
				int dx, dy;
				char dir = e.key.keysym.sym == SDLK_UP ? s.facing : turn(turn(s.facing, 1), 1);
				if (mm2_can_step(s.data, s.x, s.y, dir)) {
					mm2_facing_delta(dir, &dx, &dy);
					s.x = (s.x + dx) & 15;
					s.y = (s.y + dy) & 15;
					dirty = 1;
				}
				break;
			}
			case SDLK_PAGEUP:
			case SDLK_PAGEDOWN: {
				int m = (s.map + (e.key.keysym.sym == SDLK_PAGEUP ? MM2_MAPS - 1 : 1)) % MM2_MAPS;
				if (load_map(&s, &g, m)) dirty = 1;
				break;
			}
			default: break;
			}
		}
		if (dirty) {
			if (s.outdoors)
				mm2_view_render_outdoor(&s.view, canvas, s.data, s.x, s.y, s.facing);
			else
				mm2_view_render_indoor(&s.view, canvas, s.data, s.x, s.y, s.facing);
			{
				char line[48];
				snprintf(line, sizeof(line), "Map %d  x=%d y=%d facing %c", s.map, s.x, s.y, s.facing);
				mm2_draw_text(canvas, &font, 1, 17, line, 15, -1);
			}
			for (i = 0; i < MM2_SCREEN_W * MM2_SCREEN_H; i++)
				pixels[i] = 0xFF000000u | MM2_EGA_PALETTE[canvas[i] & 15];
			SDL_UpdateTexture(tex, NULL, pixels, MM2_SCREEN_W * 4);
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, tex, NULL, NULL);
			SDL_RenderPresent(ren);
			{
				char title[96];
				snprintf(title, sizeof(title), "Might and Magic II - map %d (%d,%d) %c", s.map, s.x, s.y, s.facing);
				SDL_SetWindowTitle(win, title);
			}
			dirty = 0;
		}
		SDL_Delay(15);
	}
	if (s.viewLoaded) mm2_view_free(&s.view);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
