/*
 * Cadaver / Cadaver: The Payoff - reimplementation front end (SDL2).
 *
 *   cadaver [-g <game dir>] [--scene logo|loading|sprites|sprites2|room|tiles|walk|intro|objects] [--level N --room R --tata --scroll Y --sim dir:ticks,...] [--screenshot out.bmp]
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
#include "engine/intro.h"
#include "engine/objsprites.h"
#include "engine/roomobjs.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { SCENE_LOGO, SCENE_LOADING, SCENE_SPRITES, SCENE_SPRITES_BIG, SCENE_ROOM, SCENE_TILES, SCENE_WALK, SCENE_INTRO, SCENE_OBJECTS, SCENE_COUNT } Scene;

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
	TileSet tiles, walls;
	ObjSprites objs;
	FloorMaps maps, wrecs;
	Mp2 mp2;
	RoomObjs ro;
	int ro_key, ro_ticks;
	uint16_t palette[16];
	int ok;
	char info[96];
} RoomView;

/* one 50 Hz timer tick: the game loop (and with it the object animation) steps every 2 ticks */
static void roomview_tick(RoomView *rv) {
	if (rv->ok && rv->ro_key && (++rv->ro_ticks & 1) == 0)
		roomobjs_step(&rv->ro);
}

static void roomview_unload(RoomView *rv) {
	rv->ro_key = 0;
	if (rv->ok) {
		tiles_free(&rv->tiles);
		tiles_free(&rv->walls);
		objsprites_free(&rv->objs);
		floormaps_free(&rv->maps);
		floormaps_free(&rv->wrecs);
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
	if (rc == 0) { rc = wallrecs_parse(f[0], fl[0], &rv->wrecs); if (rc) floormaps_free(&rv->maps); }
	if (rc == 0) rc = mp2_parse(f[1], fl[1], &rv->mp2);
	if (rc == 0) rc = tiles_load(f[2], fl[2], &rv->tiles);
	if (rc == 0) { rc = walltiles_load(f[2], fl[2], &rv->walls); if (rc) tiles_free(&rv->tiles); }
	if (rc == 0) rc = mp1_palette(f[0], fl[0], rv->palette);
	if (rc == 0) rc = objsprites_load(f[0], fl[0], &rv->objs);
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
	if (w < 3 || w > 10 || h < 3 || h > 10) {
		snprintf(rv->info, sizeof(rv->info), "%s level %d room %d: unsupported size %dx%d", rv->use_tata ? "TATA" : "DATA", rv->level, rv->room, w, h);
		return 0;
	}
	static uint8_t shell[VIEW_W * VIEW_H];
	memset(shell, TILE_TRANSPARENT, sizeof(shell));
	room_render_shell(&rv->tiles, &rv->walls, rv->maps.map[rv->room],
	                  rv->room < rv->wrecs.count ? rv->wrecs.map[rv->room] : NULL,
	                  rv->room < rv->wrecs.count ? rv->wrecs.len[rv->room] : 0, w, h, shell);
	int gx = 0, gy = 0, nobj = 0;
	if (floor_origin(w, h, &gx, &gy) == 0) {
		int key = 1 + rv->room + 100 * rv->level + 10000 * rv->use_tata;
		if (rv->ro_key != key) {
			roomobjs_build(&rv->ro, &rv->mp2, &rv->objs, rv->room, gx, gy);
			rv->ro_key = key;
		}
		roomobjs_draw(&rv->ro, shell, 1);
		nobj = rv->ro.n;
	}
	for (int i = 0; i < VIEW_W * VIEW_H; i++)
		if (shell[i] != TILE_TRANSPARENT)
			scr->pixels[i] = shell[i];
	snprintf(rv->info, sizeof(rv->info), "%s level %d room %d (%dx%d), %d objects", rv->use_tata ? "TATA" : "DATA", rv->level, rv->room, w, h, nobj);
	return 0;
}

static RoomView g_room = {1, 0, 0, 0, 0, 0, 0};

/* Tile sheet: floor tiles (60 x 32x18) then wall blocks (80 x 32x32); Up/Down scrolls. */
static void blit_tileset(Screen *scr, const TileSet *ts, int x0, int y0, int cell_h) {
	for (int k = 0; k < ts->count; k++) {
		int ox = x0 + (k % 10) * 32, oy = y0 + (k / 10) * cell_h;
		for (int y = 0; y < ts->h; y++)
			for (int x = 0; x < ts->w; x++) {
				uint8_t c = ts->pixels[(size_t)k * ts->w * ts->h + y * ts->w + x];
				int dx = ox + x, dy = oy + y;
				if (c != TILE_TRANSPARENT && dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H)
					scr->pixels[dy * SCREEN_W + dx] = c;
			}
	}
}

static int tiles_draw(RoomView *rv, const GameFiles *g, Screen *scr) {
	if (!rv->ok || rv->loaded_level != rv->level || rv->loaded_tata != rv->use_tata)
		if (roomview_load_level(rv, g) != 0)
			return -1;
	memset(scr, 0, sizeof(*scr));
	for (int i = 0; i < 16; i++)
		st_word_to_rgb(rv->palette[i], scr->palette[i]);
	memset(scr->pixels, 6, sizeof(scr->pixels));
	int fh = ((rv->tiles.count + 9) / 10) * 20;
	blit_tileset(scr, &rv->tiles, 0, -rv->scroll_y, 20);
	blit_tileset(scr, &rv->walls, 0, fh + 4 - rv->scroll_y, 32);
	snprintf(rv->info, sizeof(rv->info), "%s level %d tiles (%d floor, %d wall)", rv->use_tata ? "TATA" : "DATA", rv->level,
	         rv->tiles.count, rv->walls.count);
	return 0;
}

/* ---- walk demo: the room floor plus the animated player (no collision data yet; stays on the floor area) ---- */
enum { DIR_RIGHT, DIR_LEFT, DIR_DOWN, DIR_UP };
typedef struct {
	int x, y;            /* sprite top-left on screen */
	int dir, moving, anim, tick;
	int init_room;
	SpriteSet set;
	int set_ok;
} Walker;
static Walker g_walk;

/* PCMAN sprite numbers observed in the running game (docs/formats.md 6.11) */
static const int walk_frames[4][4] = {{0, 1, 2, 3}, {7, 6, 5, 4}, {11, 10, 9, 8}, {12, 13, 14, 15}};
static const int idle_frame[4] = {16, 21, 26, 31};

static int walker_in_room(const Walker *wk, int gx, int gy, int w, int h) {
	int h_spr = wk->set.sprites[0].h ? 42 : 42;
	double fx = wk->x + 16, fy = wk->y + h_spr - 2;
	double cx = (fx - gx) / 16.0, cy = (fy - gy - 9) / 8.0;
	double col = (cx - 1 + cy) / 2.0, row = (cy - (cx - 1)) / 2.0;
	return col >= 0 && col < w && row >= 0 && row < h;
}

/* one 50 Hz game tick; dirs: bit0 up, bit1 down, bit2 left, bit3 right (the game's own input bits) */
static void walker_tick(Walker *wk, RoomView *rv, int keys) {
	int w = 0, h = 0, gx = 0, gy = 0;
	if (rv->ok && rv->mp2.sec[0].rec[rv->room] && rv->mp2.sec[0].len[rv->room] >= 6) {
		w = rv->mp2.sec[0].rec[rv->room][4];
		h = rv->mp2.sec[0].rec[rv->room][5];
		floor_origin(w, h, &gx, &gy);
	}
	wk->tick++;
	roomview_tick(rv);
	int dx = 0, dy = 0, dir = -1;
	if (keys & 8) { dir = DIR_RIGHT; dx = 1; dy = wk->tick & 1; }
	else if (keys & 4) { dir = DIR_LEFT; dx = -1; dy = -(wk->tick & 1); }
	else if (keys & 2) { dir = DIR_DOWN; dx = -1; dy = wk->tick & 1; }
	else if (keys & 1) { dir = DIR_UP; dx = 1; dy = -(wk->tick & 1); }
	if (dir < 0) { wk->moving = 0; wk->anim = 0; return; }
	wk->dir = dir;
	Walker t = *wk;
	t.x += dx; t.y += dy;
	if (w == 0 || walker_in_room(&t, gx, gy, w, h)) { wk->x = t.x; wk->y = t.y; }
	wk->moving = 1;
	if (wk->tick % 6 == 0) wk->anim = (wk->anim + 1) & 3;
}

static int walk_draw(RoomView *rv, const GameFiles *g, Screen *scr) {
	if (!g_walk.set_ok) {
		uint8_t *data; size_t len;
		if (gamefiles_read_mjm(g, "data\\pcman.dat", -1, &data, &len) != 0) return -1;
		int rc = pcman_load(data, len, &g_walk.set);
		free(data);
		if (rc) return -1;
		g_walk.set_ok = 1;
	}
	int sx = rv->scroll_x, sy = rv->scroll_y;
	rv->scroll_x = rv->scroll_y = 0;
	if (roomview_draw(rv, g, scr) != 0) { rv->scroll_x = sx; rv->scroll_y = sy; return -1; }
	rv->scroll_x = sx; rv->scroll_y = sy;
	if (g_walk.init_room != 1 + rv->room + 100 * rv->level + 10000 * rv->use_tata) {
		int w = 10, h = 10, gx = 0, gy = 0;
		const uint8_t *rec = rv->mp2.sec[0].rec[rv->room];
		if (rec && rv->mp2.sec[0].len[rv->room] >= 6) { w = rec[4]; h = rec[5]; }
		if (floor_origin(w, h, &gx, &gy) != 0) { gx = 144; gy = 29; }
		g_walk.x = gx + 4; g_walk.y = gy + 3;      /* (148, 32) for room 0 of level 1, as in the original */
		g_walk.dir = DIR_RIGHT; g_walk.moving = 0; g_walk.anim = 0;
		g_walk.init_room = 1 + rv->room + 100 * rv->level + 10000 * rv->use_tata;
	}
	int f = g_walk.moving ? walk_frames[g_walk.dir][g_walk.anim] : idle_frame[g_walk.dir];
	const Sprite *sp = &g_walk.set.sprites[f];
	blit_sprite(scr, sp, g_walk.x + sp->hot_x, g_walk.y + sp->hot_y, 1);
	snprintf(rv->info, sizeof(rv->info), "walk demo: %s level %d room %d - arrows move along the room axes",
	         rv->use_tata ? "TATA" : "DATA", rv->level, rv->room);
	return 0;
}

/* ---- the title scroll: LOADING2.PC1 + FONT2.SPR text + animated FLAME.DAT flames ---- */
static Pc1Image g_intro_bg;
static Font g_intro_font;
static Flames g_intro_flames;
static int g_intro_ok, g_intro_acc;   /* acc = flame phase in 1/100 frames */

static int intro_draw(const GameFiles *g, Screen *scr) {
	if (!g_intro_ok) {
		uint8_t *a, *b, *c;
		size_t la, lb, lc;
		if (gamefiles_read(g, "loading2.pc1", -1, &a, &la) != 0) return -1;
		if (gamefiles_read(g, "font2.spr", -1, &b, &lb) != 0) { free(a); return -1; }
		if (gamefiles_read(g, "flame.dat", -1, &c, &lc) != 0) { free(a); free(b); return -1; }
		int rc = pc1_decode(a, la, &g_intro_bg);
		if (rc == 0) rc = font_load(b, lb, &g_intro_font);
		if (rc == 0) rc = flames_load(c, lc, &g_intro_flames);
		free(a); free(b); free(c);
		if (rc) return -1;
		g_intro_ok = 1;
	}
	intro_render_title(scr, &g_intro_bg, &g_intro_font, &g_intro_flames, g->id == GAME_PAYOFF, (g_intro_acc / 100) % 6);
	return 0;
}

/* Object sprite sheet of the current level (MP1 section 2); Up/Down scrolls. */
static int objects_draw(RoomView *rv, const GameFiles *g, Screen *scr) {
	if (!rv->ok || rv->loaded_level != rv->level || rv->loaded_tata != rv->use_tata)
		if (roomview_load_level(rv, g) != 0)
			return -1;
	memset(scr, 0, sizeof(*scr));
	for (int i = 0; i < 16; i++)
		st_word_to_rgb(rv->palette[i], scr->palette[i]);
	memset(scr->pixels, 6, sizeof(scr->pixels));
	int n = 0, cols = 8, cw = 40, ch = 48;
	for (int i = 0; i < rv->objs.count; i++)
		for (int f = 0; f < rv->objs.sprites[i].nframes; f++, n++) {
			const ObjFrame *fr = &rv->objs.sprites[i].frames[f];
			int ox = (n % cols) * cw, oy = (n / cols) * ch - rv->scroll_y;
			for (int y = 0; y < fr->h && y < ch - 2; y++)
				for (int x = 0; x < fr->w && x < cw - 2; x++) {
					uint8_t c = fr->pixels[y * fr->w + x];
					int dx = ox + x + 1, dy = oy + y + 1;
					if (c && dx >= 0 && dx < SCREEN_W && dy >= 0 && dy < SCREEN_H)
						scr->pixels[dy * SCREEN_W + dx] = c;
				}
		}
	snprintf(rv->info, sizeof(rv->info), "%s level %d: %d object sprites (%d frames)", rv->use_tata ? "TATA" : "DATA", rv->level,
	         rv->objs.count, n);
	return 0;
}

static int load_scene(const GameFiles *g, Scene s, Screen *scr) {
	if (s == SCENE_OBJECTS)
		return objects_draw(&g_room, g, scr);
	if (s == SCENE_INTRO)
		return intro_draw(g, scr);
	if (s == SCENE_WALK)
		return walk_draw(&g_room, g, scr);
	if (s == SCENE_TILES)
		return tiles_draw(&g_room, g, scr);
	if (s == SCENE_ROOM)
		return roomview_draw(&g_room, g, scr);
	if (s == SCENE_SPRITES || s == SCENE_SPRITES_BIG)
		return show_sprites(g, scr, s == SCENE_SPRITES_BIG);
	return show_pc1(g, s == SCENE_LOGO ? "logo.pc1" : "loading2.pc1", scr);
}

int main(int argc, char **argv) {
	const char *dir = ".";
	const char *shot = NULL;
	const char *sim = NULL;  /* headless walk simulation: right:30,down:10 */
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
		else if (!strcmp(argv[i], "--sim") && i + 1 < argc)
			sim = argv[++i];
		else if (!strcmp(argv[i], "--scroll") && i + 1 < argc)
			g_room.scroll_y = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
			shot = argv[++i];
		else if (!strcmp(argv[i], "--scene") && i + 1 < argc) {
			++i;
			scene = !strcmp(argv[i], "objects") ? SCENE_OBJECTS : !strcmp(argv[i], "intro") ? SCENE_INTRO : !strcmp(argv[i], "walk") ? SCENE_WALK : !strcmp(argv[i], "tiles") ? SCENE_TILES : !strcmp(argv[i], "room") ? SCENE_ROOM : !strcmp(argv[i], "loading") ? SCENE_LOADING : !strcmp(argv[i], "sprites") ? SCENE_SPRITES
			        : !strcmp(argv[i], "sprites2") ? SCENE_SPRITES_BIG : SCENE_LOGO;
		} else {
			fprintf(stderr, "usage: %s [-g gamedir] [--scene logo|loading|sprites|sprites2|room|tiles|walk|intro|objects] [--level N --room R --tata --scroll Y --sim dir:ticks,...] [--screenshot out.bmp]\n", argv[0]);
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
				else if ((scene == SCENE_ROOM || scene == SCENE_WALK) && k == SDLK_PAGEDOWN) { g_room.room++; g_room.scroll_x = g_room.scroll_y = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_WALK) && k == SDLK_PAGEUP) { g_room.room--; g_room.scroll_x = g_room.scroll_y = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_RIGHTBRACKET) { g_room.level = g_room.level % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_LEFTBRACKET) { g_room.level = (g_room.level + (g_room.use_tata ? 2 : 3)) % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_t && g.id == GAME_PAYOFF) { g_room.use_tata ^= 1; g_room.level = 1; g_room.room = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_LEFT) g_room.scroll_x -= 16;
				else if (scene == SCENE_ROOM && k == SDLK_RIGHT) g_room.scroll_x += 16;
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_OBJECTS) && k == SDLK_UP) g_room.scroll_y -= 16;
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_OBJECTS) && k == SDLK_DOWN) g_room.scroll_y += 16;
				else if (k == SDLK_SPACE || k == SDLK_RETURN)
					scene = (Scene)((scene + 1) % SCENE_COUNT);
				else
					changed = 0;
				if (changed && k != SDLK_ESCAPE) {
					load_scene(&g, scene, &scr);
					if (scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS)
						SDL_SetWindowTitle(win, g_room.info);
				}
			}
		}
		if (scene == SCENE_INTRO) {
			static Uint32 last_ms;
			Uint32 now = SDL_GetTicks();
			if (!last_ms) last_ms = now;
			while (now - last_ms >= 20) {            /* ~50 Hz; the original advances the left flame by 2 or 3 frames per tick */
				g_intro_acc += 228;
				last_ms += 20;
			}
			load_scene(&g, scene, &scr);
		}
		if (scene == SCENE_ROOM) {
			static Uint32 last_ms;
			Uint32 now = SDL_GetTicks();
			if (!last_ms) last_ms = now;
			while (now - last_ms >= 20) {
				roomview_tick(&g_room);
				last_ms += 20;
			}
			load_scene(&g, scene, &scr);
		}
		if (scene == SCENE_WALK) {
			static Uint32 last_ms;
			Uint32 now = SDL_GetTicks();
			if (!last_ms) last_ms = now;
			const Uint8 *ks = SDL_GetKeyboardState(NULL);
			int keys = (ks[SDL_SCANCODE_UP] ? 1 : 0) | (ks[SDL_SCANCODE_DOWN] ? 2 : 0) | (ks[SDL_SCANCODE_LEFT] ? 4 : 0) |
			           (ks[SDL_SCANCODE_RIGHT] ? 8 : 0);
			while (now - last_ms >= 20) {            /* the game runs at ~50 Hz */
				walker_tick(&g_walk, &g_room, keys);
				last_ms += 20;
			}
			if (sim && frames == 0) {                 /* headless: --sim right:30,down:10 */
				for (const char *c = sim; *c;) {
					int bits = !strncmp(c, "right", 5) ? 8 : !strncmp(c, "left", 4) ? 4 : !strncmp(c, "down", 4) ? 2 : !strncmp(c, "up", 2) ? 1 : 0;
					const char *colon = strchr(c, ':');
					if (!colon) break;
					for (int n = atoi(colon + 1); n > 0; n--)
						walker_tick(&g_walk, &g_room, bits);
					c = strchr(colon, ',');
					if (!c) break;
					c++;
				}
			}
			load_scene(&g, scene, &scr);
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
