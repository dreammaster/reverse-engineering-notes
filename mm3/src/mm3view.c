/* mm3view DATADIR [MAP [X Y FACING]] [--time MINUTES] [--shot out.bmp]
 * First-person view of all maps (outdoor: 41 and up) using the translated original renderer.
 * Keys: Left/Right turn, Up/Down step forward/back, Esc quits.  DATADIR holds MM3.CC, MM3.CUR and DGROUP.BIN. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recomp.h"
#include "view_glue.h"

static int wall_style(const Mm3View *v, int x, int y, int side) { return (int)(mm3_page_wall(mm3_view_page(v), x, y, side) & 7); }

/* demo rule: walls (1), lamp walls (3), gates (4) and broken walls (5) block; open, doors, open doors and arches pass */
static int passable(int style) { return !(style == 1 || style == 3 || style == 4 || style == 5); }

static int facing_back(int f) { static const int t[4] = {1, 0, 3, 2}; return t[f & 3]; }

/* Step in a direction (not changing the facing).  Indoors the wall between the cells decides; outdoors the demo allows every step
 * (terrain skills are not implemented) and mazeUpdateSlot (translated) handles page changes and the edge of the world. */
static int try_step(Mm3View *v, int dir) {
	int x = mm3_view_x(v), y = mm3_view_y(v), dx, dy;
	mm3_facing_step(dir, &dx, &dy);
	if (mm3_view_outdoor(v)) {
		mm3_view_set_party(v, x + dx, y + dy, mm3_view_facing(v));
		mm3_view_after_move(v);
		return 1;
	}
	if (!passable(wall_style(v, x, y, mm3_facing_wall_side(dir)))) return 0;
	if (x + dx < 0 || x + dx > 15 || y + dy < 0 || y + dy > 15) return 0;
	mm3_view_set_party(v, x + dx, y + dy, mm3_view_facing(v));
	return 1;
}

int main(int argc, char **argv) {
	char path[1024];
	Mm3Cc cc, cur;
	Mm3Dgroup dg;
	Mm3View *v;
	const char *shot = NULL, *dump = NULL;
	int args[5] = {1, 2, 5, 2, 0}, na = 0, quit = 0, minutes = 480;
	uint8_t screen[MM3_RAW_SIZE];
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;
	SDL_Texture *tex = NULL;
	SDL_Surface *surf;

	if (argc < 2) { fprintf(stderr, "usage: %s DATADIR [MAP [X Y FACING]] [--shot out.bmp]\n", argv[0]); return 2; }
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
		else if (!strcmp(argv[i], "--dump") && i + 1 < argc) dump = argv[++i];
		else if (!strcmp(argv[i], "--time") && i + 1 < argc) minutes = atoi(argv[++i]); /* minutes into the day */
		else if (na < 4) args[na++] = atoi(argv[i]);
	}
	snprintf(path, sizeof path, "%s/MM3.CC", argv[1]);
	if (mm3_cc_open(&cc, path)) { fprintf(stderr, "cannot open %s\n", path); return 1; }
	snprintf(path, sizeof path, "%s/MM3.CUR", argv[1]);
	if (mm3_cc_open(&cur, path)) { fprintf(stderr, "cannot open %s\n", path); return 1; }
	snprintf(path, sizeof path, "%s/DGROUP.BIN", argv[1]);
	if (mm3_dgroup_load(&dg, path)) { fprintf(stderr, "cannot open %s (make it with tools/mm3_dgroup.py)\n", path); return 1; }
	v = mm3_view_create(&cc, &cur, &dg);
	if (v) mm3_view_set_minutes(v, (unsigned)minutes);
	if (!v || mm3_view_set_map(v, (unsigned)args[0], args[1], args[2], args[3])) { fprintf(stderr, "cannot set up map %d\n", args[0]); return 1; }

	if (dump) { /* write the data segment as the view code sees it (for tools/mm3_viewcheck.py) and stop */
		FILE *f = fopen(dump, "wb");
		if (!f || fwrite(DG, 1, 65536, f) != 65536) return 1;
		fclose(f);
		return 0;
	}
	if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
	surf = SDL_CreateRGBSurfaceWithFormat(0, MM3_SCREEN_W, MM3_SCREEN_H, 32, SDL_PIXELFORMAT_ARGB8888);
	if (!shot) {
		win = SDL_CreateWindow("MM3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MM3_SCREEN_W * 3, MM3_SCREEN_H * 3, 0);
		ren = SDL_CreateRenderer(win, -1, 0);
		SDL_RenderSetLogicalSize(ren, MM3_SCREEN_W, MM3_SCREEN_H);
		tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MM3_SCREEN_W, MM3_SCREEN_H);
	}
	while (!quit) {
		SDL_Event e;
		const Mm3Palette *pal = mm3_view_palette(v);
		memset(screen, 0, sizeof screen);
		mm3_view_render(v, screen);
		for (int i = 0; i < MM3_RAW_SIZE; i++) {
			const uint8_t *c = pal->rgb[screen[i]];
			((uint32_t *)surf->pixels)[i] = 0xFF000000u | (c[0] << 16) | (c[1] << 8) | c[2];
		}
		if (shot) { SDL_SaveBMP(surf, shot); break; }
		SDL_UpdateTexture(tex, NULL, surf->pixels, surf->pitch);
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
		{
			char title[64];
			snprintf(title, sizeof title, "MM3 map %d (%d,%d) facing %d", mm3_view_map(v), (int8_t)mm3_view_x(v), (int8_t)mm3_view_y(v), mm3_view_facing(v));
			SDL_SetWindowTitle(win, title);
		}
		while (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) quit = 1;
			else if (e.type == SDL_KEYDOWN) {
				int f = mm3_view_facing(v);
				switch (e.key.keysym.sym) {
				case SDLK_LEFT: mm3_view_set_party(v, mm3_view_x(v), mm3_view_y(v), mm3_facing_left(f)); break;
				case SDLK_RIGHT: mm3_view_set_party(v, mm3_view_x(v), mm3_view_y(v), mm3_facing_right(f)); break;
				case SDLK_UP: if (!try_step(v, f)) continue; break;
				case SDLK_DOWN: if (!try_step(v, facing_back(f))) continue; break;
				default: continue;
				}
				mm3_view_toggle_animation(v);
			} else continue;
			break;
		}
	}
	SDL_Quit();
	mm3_view_destroy(v);
	return 0;
}
