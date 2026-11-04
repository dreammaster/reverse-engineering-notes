/*
 * Cadaver / Cadaver: The Payoff - reimplementation front end (SDL2).
 *
 *   cadaver [-g <game dir>] [--scene logo|loading|sprites|sprites2|room] [--level N --room R --tata] [--screenshot out.bmp]
 *
 * Current state: asset pipeline + title/loading scenes. Gameplay is not
 * implemented yet (see docs/roadmap.md).
 */
#include "engine/gamefiles.h"
#include "engine/pc1.h"
#include "engine/screen.h"
#include "engine/sprites.h"
#include "engine/mjm.h"
#include "engine/mp2.h"
#include "engine/rooms.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { SCENE_LOGO, SCENE_LOADING, SCENE_SPRITES, SCENE_SPRITES_BIG, SCENE_ROOM, SCENE_COUNT } Scene;

static int show_pc1(const GameFiles *g, const char *name, Screen *scr) {
	uint8_t *data;
	size_t len;
	if (gamefiles_read(g, name, -1, &data, &len) != 0) {
		fprintf(stderr, "cannot read %s\n", name);
		return -1;
	}
	static Pc1Image img;
	int rc = pc1_decode(data, len, &img);
	free(data);
	if (rc) {
		fprintf(stderr, "%s: decode error %d\n", name, rc);
		return -1;
	}
	memset(scr, 0, sizeof(*scr));
	memcpy(scr->pixels, img.pixels, sizeof(img.pixels));
	for (int i = 0; i < 16; i++)
		memcpy(scr->palette[i], img.palette[i], 3);
	return 0;
}

static void blit_sprite(Screen *scr, const Sprite *sp, int x, int y, int scale) {
	for (int sy = 0; sy < sp->h * scale; sy++) {
		for (int sx = 0; sx < sp->w * scale; sx++) {
			int dx = x + sx, dy = y + sy;
			uint8_t c = sp->pixels[(sy / scale) * sp->w + sx / scale];
			if (c != SPRITE_TRANSPARENT && dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H)
				scr->pixels[dy * SCREEN_W + dx] = c;
		}
	}
}

/* Player sprite sheet (PCMAN.DAT): page 0 = frames 0..39 at 1x, page 1 = frames 40..47 at 2x. */
static int show_sprites(const GameFiles *g, Screen *scr, int big) {
	uint8_t *data;
	size_t len;
	static SpriteSet set;
	if (gamefiles_read_mjm(g, "data\\pcman.dat", -1, &data, &len) != 0) {
		fprintf(stderr, "cannot read data/pcman.dat\n");
		return -1;
	}
	int rc = pcman_load(data, len, &set);
	free(data);
	if (rc) {
		fprintf(stderr, "pcman.dat: decode error %d\n", rc);
		return -1;
	}
	memset(scr, 0, sizeof(*scr));
	for (int i = 0; i < 16; i++)
		st_word_to_rgb(game_palette_st[i], scr->palette[i]);
	memset(scr->pixels, 12, sizeof(scr->pixels));
	if (!big) {
		for (int i = 0; i < 40 && i < set.count; i++)
			blit_sprite(scr, &set.sprites[i], (i % 10) * 32, (i / 10) * 48 + 2, 1);
	} else {
		for (int i = 40; i < set.count; i++)
			blit_sprite(scr, &set.sprites[i], ((i - 40) % 4) * 80, ((i - 40) / 4) * 98 + 2, 2);
	}
	spriteset_free(&set);
	return 0;
}

/* ---- room floor viewer: PgUp/PgDn = room, [ ] = level, T = DATA/TATA set (Payoff), arrows = scroll ---- */
typedef struct {
	int level, room, use_tata, scroll_x, scroll_y;
	int loaded_level, loaded_tata;
	TileSet tiles;
	FloorMaps maps;
	Mp2 mp2;
	uint16_t palette[16];
	int ok;
	char info[96];
} RoomView;

static void roomview_unload(RoomView *rv) {
	if (rv->ok) {
		tiles_free(&rv->tiles);
		floormaps_free(&rv->maps);
		mp2_free(&rv->mp2);
	}
	rv->ok = 0;
}

static int roomview_load_level(RoomView *rv, const GameFiles *g) {
	roomview_unload(rv);
	const char *set = rv->use_tata ? "tata" : "data";
	char name[64];
	uint8_t *f[3] = {0, 0, 0};
	size_t fl[3] = {0, 0, 0};
	for (int i = 0; i < 3; i++) {
		snprintf(name, sizeof(name), "%s\\?\\lev?.mp%d", set, i + 1);
		if (gamefiles_read_mjm(g, name, rv->level, &f[i], &fl[i]) != 0) {
			fprintf(stderr, "cannot read %s (level %d)\n", name, rv->level);
			for (int k = 0; k < i; k++) free(f[k]);
			return -1;
		}
	}
	int rc = floormaps_parse(f[0], fl[0], &rv->maps);
	if (rc == 0) rc = mp2_parse(f[1], fl[1], &rv->mp2);
	if (rc == 0) rc = tiles_load(f[2], fl[2], &rv->tiles);
	if (rc == 0) rc = mp1_palette(f[0], fl[0], rv->palette);
	for (int i = 0; i < 3; i++) free(f[i]);
	if (rc != 0) {
		fprintf(stderr, "level %d: decode error %d\n", rv->level, rc);
		return -1;
	}
	rv->ok = 1;
	rv->loaded_level = rv->level;
	rv->loaded_tata = rv->use_tata;
	return 0;
}

static int roomview_draw(RoomView *rv, const GameFiles *g, Screen *scr) {
	if (!rv->ok || rv->loaded_level != rv->level || rv->loaded_tata != rv->use_tata)
		if (roomview_load_level(rv, g) != 0)
			return -1;
	memset(scr, 0, sizeof(*scr));
	for (int i = 0; i < 16; i++)
		st_word_to_rgb(rv->palette[i], scr->palette[i]);
	memset(scr->pixels, 6, sizeof(scr->pixels));
	int count = rv->maps.count < rv->mp2.sec[0].count ? rv->maps.count : rv->mp2.sec[0].count;
	if (rv->room < 0) rv->room = 0;
	if (rv->room >= count) rv->room = count - 1;
	int w = 0, h = 0;
	const uint8_t *rec = rv->mp2.sec[0].rec[rv->room];
	if (rec && rv->mp2.sec[0].len[rv->room] >= 6) { w = rec[4]; h = rec[5]; }
	if (!rv->maps.map[rv->room] || w * h != rv->maps.len[rv->room]) {
		snprintf(rv->info, sizeof(rv->info), "%s level %d room %d: no floor map", rv->use_tata ? "TATA" : "DATA", rv->level, rv->room);
		return 0;
	}
	int cw, ch;
	floor_canvas_size(w, h, &cw, &ch);
	uint8_t *cv = (uint8_t *)malloc((size_t)cw * ch);
	if (!cv) return -1;
	memset(cv, TILE_TRANSPARENT, (size_t)cw * ch);
	floor_render(&rv->tiles, rv->maps.map[rv->room], w, h, cv);
	int ox = (SCREEN_W - cw) / 2 - rv->scroll_x, oy = (SCREEN_H - ch) / 2 - rv->scroll_y;
	for (int y = 0; y < ch; y++)
		for (int x = 0; x < cw; x++) {
			int dx = ox + x, dy = oy + y;
			if (cv[y * cw + x] != TILE_TRANSPARENT && dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H)
				scr->pixels[dy * SCREEN_W + dx] = cv[y * cw + x];
		}
	free(cv);
	snprintf(rv->info, sizeof(rv->info), "%s level %d room %d (%dx%d)", rv->use_tata ? "TATA" : "DATA", rv->level, rv->room, w, h);
	return 0;
}

static RoomView g_room = {1, 0, 0, 0, 0, 0, 0};

static int load_scene(const GameFiles *g, Scene s, Screen *scr) {
	if (s == SCENE_ROOM)
		return roomview_draw(&g_room, g, scr);
	if (s == SCENE_SPRITES || s == SCENE_SPRITES_BIG)
		return show_sprites(g, scr, s == SCENE_SPRITES_BIG);
	return show_pc1(g, s == SCENE_LOGO ? "logo.pc1" : "loading2.pc1", scr);
}

int main(int argc, char **argv) {
	const char *dir = ".";
	const char *shot = NULL;
	Scene scene = SCENE_LOGO;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-g") && i + 1 < argc)
			dir = argv[++i];
		else if (!strcmp(argv[i], "--level") && i + 1 < argc)
			g_room.level = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--room") && i + 1 < argc)
			g_room.room = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--tata"))
			g_room.use_tata = 1;
		else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
			shot = argv[++i];
		else if (!strcmp(argv[i], "--scene") && i + 1 < argc) {
			++i;
			scene = !strcmp(argv[i], "room") ? SCENE_ROOM : !strcmp(argv[i], "loading") ? SCENE_LOADING : !strcmp(argv[i], "sprites") ? SCENE_SPRITES
			        : !strcmp(argv[i], "sprites2") ? SCENE_SPRITES_BIG : SCENE_LOGO;
		} else {
			fprintf(stderr, "usage: %s [-g gamedir] [--scene logo|loading|sprites|sprites2|room] [--level N --room R --tata] [--screenshot out.bmp]\n", argv[0]);
			return 2;
		}
	}

	GameFiles g;
	if (gamefiles_open(&g, dir) != 0) {
		fprintf(stderr, "no Cadaver game found in '%s'\n", dir);
		return 1;
	}
	printf("Found: %s\n", gamefiles_name(g.id));

	static Screen scr;
	static uint32_t rgb[SCREEN_W * SCREEN_H];
	if (load_scene(&g, scene, &scr) != 0)
		return 1;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Window *win = SDL_CreateWindow(gamefiles_name(g.id), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                   SCREEN_W * 3, SCREEN_H * 3, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *ren = win ? SDL_CreateRenderer(win, -1, 0) : NULL;
	SDL_Texture *tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
	                                           SCREEN_W, SCREEN_H) : NULL;
	if (!tex) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);

	int running = 1, frames = 0;
	while (running) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				running = 0;
			else if (ev.type == SDL_KEYDOWN) {
				SDL_Keycode k = ev.key.keysym.sym;
				int changed = 1;
				if (k == SDLK_ESCAPE)
					running = 0;
				else if (scene == SCENE_ROOM && k == SDLK_PAGEDOWN) { g_room.room++; g_room.scroll_x = g_room.scroll_y = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_PAGEUP) { g_room.room--; g_room.scroll_x = g_room.scroll_y = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_RIGHTBRACKET) { g_room.level = g_room.level % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_LEFTBRACKET) { g_room.level = (g_room.level + (g_room.use_tata ? 2 : 3)) % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_t && g.id == GAME_PAYOFF) { g_room.use_tata ^= 1; g_room.level = 1; g_room.room = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_LEFT) g_room.scroll_x -= 16;
				else if (scene == SCENE_ROOM && k == SDLK_RIGHT) g_room.scroll_x += 16;
				else if (scene == SCENE_ROOM && k == SDLK_UP) g_room.scroll_y -= 16;
				else if (scene == SCENE_ROOM && k == SDLK_DOWN) g_room.scroll_y += 16;
				else if (k == SDLK_SPACE || k == SDLK_RETURN)
					scene = (Scene)((scene + 1) % SCENE_COUNT);
				else
					changed = 0;
				if (changed && k != SDLK_ESCAPE) {
					load_scene(&g, scene, &scr);
					if (scene == SCENE_ROOM)
						SDL_SetWindowTitle(win, g_room.info);
				}
			}
		}
		screen_to_rgb32(&scr, rgb);
		SDL_UpdateTexture(tex, NULL, rgb, SCREEN_W * 4);
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
		if (shot && ++frames == 2) {
			SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(rgb, SCREEN_W, SCREEN_H, 32, SCREEN_W * 4,
			                                                    SDL_PIXELFORMAT_RGB888);
			SDL_SaveBMP(s, shot);
			SDL_FreeSurface(s);
			running = 0;
		}
		SDL_Delay(16);
	}
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
