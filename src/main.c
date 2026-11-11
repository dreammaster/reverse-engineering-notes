/*
 * Cadaver / Cadaver: The Payoff - reimplementation front end (SDL2).
 *
 *   cadaver [-g <game dir>] [--scene game|logo|loading|sprites|sprites2|room|tiles|walk|intro|objects] [--level N --room R --tata --scroll Y --sim dir:ticks,...] [--screenshot out.bmp]
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
#include "engine/collision.h"
#include "engine/roomexits.h"
#include "engine/panel.h"
#include "engine/push.h"
#include "engine/acl.h"
#include "engine/lng.h"
#include "engine/message.h"
#include "engine/tad.h"

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
typedef struct RoomView_ {
	int level, room, use_tata, scroll_x, scroll_y;
	int loaded_level, loaded_tata;
	TileSet tiles, walls;
	ObjSprites objs;
	FloorMaps maps, wrecs;
	Mp2 mp2;
	RoomObjs ro;
	int ro_key, ro_ticks;
	uint8_t visited[128];      /* rooms entered so far (event 28 fires on the first visit only) */
	int tm_div, tm_period, tm_cnt;   /* room timer: every 17 game steps count down rv->tm_cnt, at 0 fire event 14 (period = room record byte 2) */
	uint8_t flags_seen[ACL_THINGS];  /* thing flags the objects were built with */
	uint8_t room_seen[ACL_THINGS];   /* ... and their rooms / positions */
	uint8_t pos_seen[ACL_THINGS * 3];
	int pending_rebuild;
	struct { int active, id, timer; } msgs[4];   /* message scrolls (slots 0..3) */
	uint8_t verbs[VERB_SLOTS];  /* verb icons of the touched object (slots) */
	int nverbs;
	int energy_max_cfg;        /* from LEVn.MP1 */
	int touch_age, menu_open, menu_sel, menu_delay, menu_released, touch_thing, end_prev;   /* verb menu (End held / pressed while touching an object) */
	int carry_pending, carry_energy; uint32_t carry_money, carry_score;   /* stats carried into the next level */
	int restarting;            /* level torn down after a death; the next draw reloads it */
	int dead_timer;            /* > 0: the player died (energy 0), counts game steps until the level restarts */
	RankTable ranks; int ranks_ok; unsigned six_fa;   /* status dialog data (MP1) */
	int show_status;           /* 'I': the character status dialog */
	int label_id;              /* name shown above the room name: string id of the last touched object (-1 none) */
	AclVM vm;                  /* script state: thing flags / variables */
	TadRt tad;                 /* native routines of ROUTINEn.TAD (timers) */
	int vm_ok;
	uint16_t palette[16];
	uint16_t name_tbl[20];     /* room-name string ids (MP1 trailer) */
	LngTable names;            /* ENGn.LNG */
	int names_ok;
	int ok;
	char info[96];
} RoomView;

/* one 50 Hz timer tick: the game loop (and with it the object animation) steps every 2 ticks */
static int room_key(const RoomView *rv) { return 1 + rv->room + 100 * rv->level + 10000 * rv->use_tata; }
static int game_rand(int lo, int hi, void *u) {
	(void)u;
	static unsigned s = 12345;
	s = s * 1103515245u + 12345u;
	return hi <= lo ? lo : lo + (int)((s >> 16) % (unsigned)(hi - lo + 1));
}
static void roomview_game_step(RoomView *rv);
static void walk_refresh_world(RoomView *rv);
static void roomview_ensure_objs(RoomView *rv, int gx, int gy);
static void roomview_tick(RoomView *rv) {
	if (rv->ok && rv->ro_key && (++rv->ro_ticks & 1) == 0) {
		roomobjs_step(&rv->ro);
		roomview_game_step(rv);
	}
}

/* the frame around the room view (ANDATA.DAT block 1) */
static uint8_t *g_andata;
static const uint8_t *g_b1, *g_font, *g_tiles, *g_font3, *g_icons;
static size_t g_b1_len, g_font_len, g_tiles_len, g_font3_len, g_icons_len;
static void panel_ensure(const GameFiles *g) {
	if (g_andata)
		return;
	size_t len;
	if (gamefiles_read_mjm(g, "data\\andata.dat", -1, &g_andata, &len) == 0)
	{
		andata_block(g_andata, len, 1, &g_b1, &g_b1_len);
		andata_block(g_andata, len, 4, &g_font, &g_font_len);
		andata_block(g_andata, len, 2, &g_tiles, &g_tiles_len);
		andata_block(g_andata, len, 3, &g_font3, &g_font3_len);
		andata_block(g_andata, len, 0, &g_icons, &g_icons_len);
	}
}
static int msg_rnd(void *u) { (void)u; return 3; }
/* wings + the status text ("DAY n" and the room name) */
static void panel_draw(const GameFiles *g, Screen *scr, const RoomView *rv) {
	panel_ensure(g);
	if (g_b1)
		panel_apply_wings(g_b1, g_b1_len, scr->pixels);
	energy_bar_draw(scr->pixels, rv->vm.energy, rv->vm.energy_max);
	if (rv->show_status && g_tiles && g_font3 && rv->names_ok && rv->ranks_ok) {
		unsigned vis = 0;
		for (int i = 0; i < 128; i++) vis += rv->visited[i];
		unsigned w5 = rv->tad.loaded ? (unsigned)(rv->tad.mem[0x30000 + 0x36] | (rv->tad.mem[0x30000 + 0x37] << 8)) : 2;
		int rid = rank_string_id(&rv->ranks, rv->vm.score);
		status_dialog_draw(scr->pixels, g_tiles, g_tiles_len, g_font3, g_font3_len, rv->names.count > 13 ? rv->names.strings[13] : "",
		                   rv->vm.energy, rv->vm.energy_max, rv->vm.money, rv->vm.score, rv->level, complete_percent(vis, rv->six_fa, w5),
		                   rid < rv->names.count ? rv->names.strings[rid] : "", msg_rnd, NULL);
	}
	if (g_icons && rv->nverbs) {
		verbs_draw(scr->pixels, rv->verbs, rv->nverbs, g_icons, g_icons_len);
		if (rv->menu_open)
			verbs_highlight(scr->pixels, rv->menu_sel);
	}
	if (g_font && rv->names_ok && rv->room >= 0 && rv->room < rv->mp2.sec[0].count && rv->mp2.sec[0].rec[rv->room]) {
		char nm[48];
		status_room_name(rv->mp2.sec[0].rec[rv->room], (size_t)rv->mp2.sec[0].len[rv->room], rv->name_tbl, rv->names.strings,
		                 rv->names.count, nm, sizeof(nm));
		status_draw(scr->pixels, g_font, g_font_len, nm, 1);
		for (int sl = 0; sl < 4 && g_tiles && g_font3; sl++)
			if (rv->msgs[sl].active && rv->msgs[sl].id >= 0 && rv->msgs[sl].id < rv->names.count)
				message_draw(scr->pixels, rv->names.strings[rv->msgs[sl].id], sl, g_tiles, g_tiles_len, g_font3, g_font3_len, msg_rnd, NULL);
		if (rv->label_id > 0 && rv->label_id < rv->names.count) {
			char lb[16];
			snprintf(lb, sizeof(lb), "%.12s", rv->names.strings[rv->label_id]);
			status_draw_centred(scr->pixels, 229, 172, 84, lb, g_font, g_font_len);
		}
	}
}

/* while the walk demo runs, objects in front of the player are drawn after it (see walk_draw) */
static struct { int on, x1, y1, z0, z1; } g_plbox;

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
	rv->energy_max_cfg = level_energy_max(f[0], fl[0]);
	rv->ranks_ok = rank_table_load(f[0], fl[0], &rv->ranks) == 0;
	rv->six_fa = fl[0] > 66 ? (unsigned)((f[0][64] << 8) | f[0][65]) : 72;
	if (rv->names_ok) { lng_free(&rv->names); rv->names_ok = 0; }
	if (rc == 0 && status_name_table(f[0], fl[0], rv->name_tbl) == 0) {
		uint8_t *lg; size_t ll;
		if (gamefiles_read_mjm(g, rv->use_tata ? "tata\\?\\eng?.lng" : "data\\?\\eng?.lng", rv->level, &lg, &ll) == 0) {
			rv->names_ok = lng_parse(lg, ll, &rv->names) == 0;
			free(lg);
		}
	}
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
	acl_init(&rv->vm, &rv->mp2);
	rv->vm.rand_range = game_rand;
	if (rv->tad.loaded) tad_free(&rv->tad);
	{
		uint8_t *tb; size_t tl;
		const char *pat[2] = {rv->use_tata ? "tata\\?\\routine?.tad" : "data\\?\\routine?.tad", rv->use_tata ? "tata\\?\\routine?.pak" : "data\\?\\routine?.pak"};
		for (int k = 0; k < 2 && !rv->tad.loaded; k++)      /* Cadaver 1: .TAD, Cadaver 2: .PAK (same packing, same code for levels 1-5) */
			if (gamefiles_read_mjm(g, pat[k], rv->level, &tb, &tl) == 0) {
				tad_load(&rv->tad, tb, tl);
				free(tb);
			}
	}
	rv->vm.energy = rv->vm.energy_max = rv->energy_max_cfg;
	if (getenv("CAD_ENERGY")) rv->vm.energy = atoi(getenv("CAD_ENERGY"));     /* debugging aid: start with this energy */
	memset(rv->visited, 0, sizeof(rv->visited));
	if (rv->carry_pending) {
		rv->carry_pending = 0;
		rv->vm.money = rv->carry_money; rv->vm.score = rv->carry_score;
		rv->vm.energy = rv->carry_energy > rv->vm.energy_max ? rv->vm.energy_max : rv->carry_energy;
	}
	rv->vm_ok = 1;
	rv->restarting = 0;
	rv->dead_timer = 0;
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
	memset(scr->pixels, 0, sizeof(scr->pixels));
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
	room_render_shell_doors(&rv->tiles, &rv->walls, &rv->objs, rv->maps.map[rv->room],
	                  rv->room < rv->wrecs.count ? rv->wrecs.map[rv->room] : NULL,
	                  rv->room < rv->wrecs.count ? rv->wrecs.len[rv->room] : 0, w, h, shell);
	int gx = 0, gy = 0, nobj = 0;
	if (floor_origin(w, h, &gx, &gy) == 0) {
		roomview_ensure_objs(rv, gx, gy);
		if (g_plbox.on)
			roomobjs_draw_rel(&rv->ro, shell, 1, g_plbox.x1, g_plbox.y1, g_plbox.z0, g_plbox.z1, 0);
		else
			roomobjs_draw(&rv->ro, shell, 1);
		nobj = rv->ro.n;
	}
	for (int i = 0; i < VIEW_W * VIEW_H; i++)
		if (shell[i] != TILE_TRANSPARENT)
			scr->pixels[i] = shell[i];
	if (!g_plbox.on)
		panel_draw(g, scr, rv);
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

/* ---- walk demo: the player moves in 1/8-cell units with the game's own collision test (engine/collision.c), room exits
 * (engine/roomexits.c) lead to the next room; objects are depth-sorted against the player approximately ---- */
enum { DIR_RIGHT, DIR_LEFT, DIR_DOWN, DIR_UP };
typedef struct {
	int x1, y1, z;       /* far corner of the player's 7x7x30 box, 1/8 cell units */
	int dir, moving, anim, tick;
	int world_key, placed;
	ColWorld world;
	ColBox objs[ROOMOBJS_MAX];
	ColBox exits[ROOM_MAX_EXITS];
	uint8_t hdr8[32];
	int hdr8_ok;
	SpriteSet set;
	int set_ok;
} Walker;
static Walker g_walk;
static void walker_world(Walker *wk, RoomView *rv);
static void walk_refresh_world(RoomView *rv) {
	if (g_walk.world_key)
		walker_world(&g_walk, rv);
}

#define PL_XS 7
#define PL_YS 7
#define PL_ZS 30

/* PCMAN sprite numbers observed in the running game (docs/formats.md 6.11) */
static const int walk_frames[4][4] = {{0, 1, 2, 3}, {7, 6, 5, 4}, {11, 10, 9, 8}, {12, 13, 14, 15}};
static const int idle_frame[4] = {16, 21, 26, 31};

static int roomview_geom(RoomView *rv, int *w, int *h, int *gx, int *gy) {
	const uint8_t *rec = rv->mp2.sec[0].rec[rv->room];
	if (!rv->ok || !rec || rv->mp2.sec[0].len[rv->room] < 22)
		return 0;
	*w = rec[4]; *h = rec[5];
	return *w >= 3 && *w <= 10 && *h >= 3 && *h <= 10 && floor_origin(*w, *h, gx, gy) == 0;
}



static int rv_changed(const RoomView *rv) {
	return memcmp(rv->flags_seen, rv->vm.flags, sizeof(rv->flags_seen)) || memcmp(rv->room_seen, rv->vm.room, sizeof(rv->room_seen)) ||
	       memcmp(rv->pos_seen, rv->vm.pos, sizeof(rv->pos_seen));
}
static void rv_remember(RoomView *rv) {
	memcpy(rv->flags_seen, rv->vm.flags, sizeof(rv->flags_seen));
	memcpy(rv->room_seen, rv->vm.room, sizeof(rv->room_seen));
	memcpy(rv->pos_seen, rv->vm.pos, sizeof(rv->pos_seen));
}

/* show the messages queued by script ops (op 28): first free slot, else the one that expires first (1000:3555) */
static void roomview_flush_messages(RoomView *rv) {
	for (int k = 0; k < rv->vm.msg_n; k++) {
		int sl = -1, best = 1 << 30;
		for (int i = 0; i < 4; i++)           /* the same string is already shown: only its timer restarts */
			if (rv->msgs[i].active && rv->msgs[i].id == rv->vm.msg[k]) { sl = i; break; }
		if (sl >= 0) {
			rv->msgs[sl].timer = MSG_STEPS;
			continue;
		}
		for (int i = 0; i < 4; i++) {
			if (!rv->msgs[i].active) { sl = i; break; }
			if (rv->msgs[i].timer < best) { best = rv->msgs[i].timer; sl = i; }
		}
		rv->msgs[sl].active = 1;
		rv->msgs[sl].id = rv->vm.msg[k];
		rv->msgs[sl].timer = MSG_STEPS;
	}
	rv->vm.msg_n = 0;
}

/* the script flags changed (things hidden / placed): rebuild the objects, keeping the positions and animations of those that stay */
static void roomview_rebuild_objs(RoomView *rv, int gx, int gy) {
	static RoomObjs old;
	memcpy(&old, &rv->ro, sizeof(old));
	roomobjs_build_ex(&rv->ro, &rv->mp2, &rv->objs, rv->room, gx, gy, rv->vm.flags, rv->vm.room, rv->vm.pos);
	for (int i = 0; i < rv->ro.n; i++)
		for (int k = 0; k < old.n; k++)
			if (old.o[k].thing == rv->ro.o[i].thing) {
				int t3 = 3 * rv->ro.o[i].thing;
				if (memcmp(rv->vm.pos + t3, rv->pos_seen + t3, 3) != 0) break;       /* a script moved it: keep the new position */
				rv->ro.o[i].x1 = old.o[k].x1; rv->ro.o[i].y1 = old.o[k].y1;
				rv->ro.o[i].an = old.o[k].an;
				roomobj_moved(&rv->ro.o[i], gx, gy);
				break;
			}
	rv_remember(rv);
	walk_refresh_world(rv);          /* collision world must be rebuilt (the player keeps its position) */
}

/* one game step (25 Hz): the room timers of 1000:11EB5 (units of 17 steps) -> events 14 and 20 */
static void roomview_game_step(RoomView *rv) {
	for (int i = 0; i < 4; i++)
		if (rv->msgs[i].active && --rv->msgs[i].timer <= 0)
			rv->msgs[i].active = 0;
	if (rv->room < 0 || rv->room >= 128 || --rv->tm_div > 0)
		return;
	rv->tm_div = 17;
	tad_timer_unit(&rv->tad, &rv->vm);
	if (rv->tm_period && --rv->tm_cnt <= 0) {
		rv->tm_cnt = rv->tm_period;
		acl_room_event(&rv->vm, &rv->mp2, rv->room, 14, 0);
	}
	if (rv->vm.timer_delay >= 0 && --rv->vm.timer_delay == 0) {
		int a = rv->vm.timer_arg;
		rv->vm.timer_delay = -1;
		acl_room_event(&rv->vm, &rv->mp2, rv->room, 20, a);
	}
	rv->pending_rebuild = rv_changed(rv);
	roomview_flush_messages(rv);
}

/* entering a room: first visit event (28), entry event (6), timer set up from room record byte 2 (1000:DA60) */
static void roomview_enter(RoomView *rv, int gx, int gy) {
	rv->vm.room_no = rv->room;
	if (rv->room >= 0 && rv->room < 128 && !rv->visited[rv->room]) {
		rv->visited[rv->room] = 1;
		acl_room_event(&rv->vm, &rv->mp2, rv->room, 28, 0);
	}
	acl_room_event(&rv->vm, &rv->mp2, rv->room, 6, 0);
	roomview_flush_messages(rv);
	const uint8_t *rec = rv->mp2.sec[0].rec[rv->room];
	rv->tm_period = rv->tm_cnt = rec && rv->mp2.sec[0].len[rv->room] > 2 ? rec[2] : 0;
	rv->tm_div = 17;
	rv->vm.timer_delay = -1;
	roomobjs_build_ex(&rv->ro, &rv->mp2, &rv->objs, rv->room, gx, gy, rv->vm.flags, rv->vm.room, rv->vm.pos);
	rv_remember(rv);
	rv->ro_key = room_key(rv);
	rv->label_id = -1;
	rv->nverbs = 0;
}

static void roomview_ensure_objs(RoomView *rv, int gx, int gy) {
	if (rv->ro_key != room_key(rv))
		roomview_enter(rv, gx, gy);
	else if (rv->pending_rebuild) {
		rv->pending_rebuild = 0;
		roomview_rebuild_objs(rv, gx, gy);
	}
}

static void walker_world(Walker *wk, RoomView *rv) {
	int w, h, gx, gy;
	if (!roomview_geom(rv, &w, &h, &gx, &gy))
		return;
	roomview_ensure_objs(rv, gx, gy);
	const uint8_t *rec = rv->mp2.sec[0].rec[rv->room];
	ColWorld *cw = &wk->world;
	memset(cw, 0, sizeof(*cw));
	cw->w8 = w * 8; cw->h8 = h * 8;
	cw->thick_y = rec[20]; cw->thick_x = rec[21];
	cw->wallx = 2; cw->wally = 10;               /* constant in all 72 rooms of level 1 (docs 6.12) */
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		ColBox *b = &wk->objs[i];
		b->x1 = (int8_t)o->cx1; b->y1 = (int8_t)o->cy1; b->x0 = (int8_t)o->cx0; b->y0 = (int8_t)o->cy0;
		b->zt = (int8_t)o->czt; b->zb = (int8_t)o->czb;
		b->solid = o->solid;
		b->id = o->thing;
	}
	cw->objs = wk->objs; cw->nobjs = rv->ro.n;
	cw->nexits = room_build_exits(&rv->mp2, &rv->objs, rv->room, wk->exits);
	cw->exits = wk->exits;
	wk->world_key = room_key(rv);
	if (!wk->hdr8_ok) {
		const uint8_t *hd = objsprites_header(&rv->objs, 8);
		if (hd) { memcpy(wk->hdr8, hd, 32); wk->hdr8_ok = 1; }
	}
}

/* copy the current object boxes into the collision world (objects move when pushed) */
static void walker_sync_boxes(Walker *wk, RoomView *rv) {
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		ColBox *b = &wk->objs[i];
		b->x1 = (int8_t)o->cx1; b->y1 = (int8_t)o->cy1; b->x0 = (int8_t)o->cx0; b->y0 = (int8_t)o->cy0;
		b->zt = (int8_t)o->czt; b->zb = (int8_t)o->czb;
	}
}

/* free spot near (cx, cy): spiral search for a position where the player's box overlaps nothing and is inside the room */
static int walker_find_free(Walker *wk, int cx, int cy) {
	for (int r = 0; r < 40; r++)
		for (int dy = -r; dy <= r; dy++)
			for (int dx = -r; dx <= r; dx++) {
				if (abs(dx) != r && abs(dy) != r)
					continue;
				ColResult cr;
				collision_test(&wk->world, cx + dx, cy + dy, 0, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
				if (cr.zf) {
					wk->x1 = cx + dx; wk->y1 = cy + dy; wk->z = 0;
					return 1;
				}
			}
	return 0;
}

static void walker_place_default(Walker *wk, RoomView *rv) {
	/* level 1 room 0: thing 0 (the player) is in the room list at (24, 23) */
	const uint8_t *list = rv->mp2.sec[2].rec[rv->room];
	int n = list ? rv->mp2.sec[2].len[rv->room] / 2 : 0;
	for (int i = 0; i < n; i++)
		if (((list[2 * i] << 8) | list[2 * i + 1]) == 0) {
			int len;
			const uint8_t *t = mp2_thing(&rv->mp2, 0, &len);
			if (t && len >= 3) { wk->x1 = t[0]; wk->y1 = t[1]; wk->z = t[2]; return; }
		}
	walker_find_free(wk, wk->world.w8 / 2, wk->world.h8 / 2);
}

/* the other room that lists door `d` */
static int door_target(const RoomView *rv, int d) {
	for (int r = 0; r < rv->mp2.sec[0].count; r++) {
		const uint8_t *rec = rv->mp2.sec[0].rec[r];
		if (r == rv->room || !rec || rv->mp2.sec[0].len[r] < 22)
			continue;
		for (int i = 0; i < 7; i++)
			if (((rec[6 + 2 * i] << 8) | rec[7 + 2 * i]) == d)
				return r;
	}
	return -1;
}

static void walker_enter(Walker *wk, RoomView *rv, int room, int door) {
	rv->room = room;
	wk->world_key = 0;
	walker_world(wk, rv);
	int placed = 0;
	for (int i = 0; i < wk->world.nexits && !placed; i++)
		if (wk->exits[i].id == door) {
			int lx, ly;
			room_landing(&rv->mp2, room, &wk->exits[i], &lx, &ly);
			ColResult cr;
			collision_test(&wk->world, lx, ly, 0, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
			if (cr.zf) { wk->x1 = lx; wk->y1 = ly; wk->z = 0; placed = 1; }
			else placed = walker_find_free(wk, lx, ly);
		}
	if (!placed)
		walker_find_free(wk, wk->world.w8 / 2, wk->world.h8 / 2);
}

/* one 50 Hz timer tick; keys: bit0 up, bit1 down, bit2 left, bit3 right (the game's own input bits). The game loop (and
 * the player's move of one unit) runs every 2 ticks. */
/* the player pressed against objects (1000:22C4): the first object that is not "ignored" (header flag 0x40) gets its name shown (header word 5 =
 * string id, 12 characters) and receives the events 9 (argument 0) and 7; queued room events are delivered afterwards */
static void walker_touch(Walker *wk, RoomView *rv, const ColResult *cr, int keys) {
	(void)wk;
	for (int k = 0; k < cr->n; k++) {
		const RoomObj *o = NULL;
		for (int i = 0; i < rv->ro.n; i++)
			if (rv->ro.o[i].thing == cr->ids[k]) { o = &rv->ro.o[i]; break; }
		if (!o || (o->sp->header[12] & 0x40))
			continue;
		int nm = (o->sp->header[10] << 8) | o->sp->header[11];
		rv->label_id = nm < 0x8000 ? nm : -1;
		rv->nverbs = verbs_list(o->sp->header, rv->vm.flags2[o->thing], 0, rv->verbs);
		rv->touch_thing = o->thing;
		rv->touch_age = 0;
		if ((keys & 0x80) && !rv->menu_open) {         /* End while touching: the verb menu opens (1000:244C -> 2A33) */
			rv->menu_open = 1; rv->menu_sel = 0; rv->menu_delay = 0; rv->menu_released = 0;
			return;
		}
		rv->vm.queue_n = 0;
		acl_thing_event(&rv->vm, &rv->mp2, o->thing, 9, 0);
		acl_thing_event(&rv->vm, &rv->mp2, o->thing, 7, 0);
		for (int q = 0; q < rv->vm.queue_n; q++)
			acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
		rv->pending_rebuild |= rv_changed(rv);
		roomview_flush_messages(rv);
		return;
	}
}

/* run the chosen verb (handler table 1000:2E0B): 11 = look (event 16 when the thing has flag 0x20, else message 3), 4 and 7 = event 5, 2 = event 0;
 * the other verbs (take, drop, use ...) are not decoded yet */
static void menu_choose(RoomView *rv, int verb) {
	int t = rv->touch_thing;
	rv->vm.queue_n = 0;
	if (verb == 11) {
		if (t >= 0 && t < ACL_THINGS && (rv->vm.flags[t] & 0x20)) acl_thing_event(&rv->vm, &rv->mp2, t, 16, 0);
		else if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 3;
	} else if (verb == 4 || verb == 7) {
		acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
	} else if (verb == 2) {
		acl_thing_event(&rv->vm, &rv->mp2, t, 0, 0);
	} else if (verb == 14) {          /* 1000:3483: class 7 things get event 5, everything else is removed (queued for deletion, 1000:62A9) */
		const RoomObj *o = NULL;
		for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == t) { o = &rv->ro.o[i]; break; }
		if (o && o->sp->header[0x17] == 7) acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
		else if (t >= 0 && t < ACL_THINGS) rv->vm.flags[t] |= 0x80;
	}
	for (int q = 0; q < rv->vm.queue_n; q++)
		acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
}

/* one game step with the verb menu open (1000:2A33): Left/Right move the highlight (every 13 steps while held), End chooses, Space/End-less cancel */
static void menu_step(RoomView *rv, int keys, int end_edge) {
	if (!(keys & 0x80)) rv->menu_released = 1;
	if (rv->menu_delay > 0) rv->menu_delay--;
	if (end_edge && rv->menu_released) {
		int verb = rv->verbs[rv->menu_sel];
		rv->menu_open = 0;
		menu_choose(rv, verb);
		return;
	}
	if (rv->menu_delay == 0 && rv->nverbs > 0) {
		if (keys & 8) { rv->menu_sel = (rv->menu_sel + 1) % rv->nverbs; rv->menu_delay = 13; }
		else if (keys & 4) { rv->menu_sel = (rv->menu_sel + rv->nverbs - 1) % rv->nverbs; rv->menu_delay = 13; }
	}
}

/* op 37: put the player into `room` at (x + rec[1], y + rec[3], z) like 1000:7F3C; z grows until the spot is free */
static void walker_teleport(Walker *wk, RoomView *rv, int room, int x, int y, int z) {
	const uint8_t *rec = room >= 0 && room < rv->mp2.sec[0].count ? rv->mp2.sec[0].rec[room] : NULL;
	if (!rec || rv->mp2.sec[0].len[room] < 6)
		return;
	rv->room = room;
	wk->world_key = 0;
	walker_world(wk, rv);
	int lx = (x + rec[1]) & 0xFF, ly = (y + rec[3]) & 0xFF, lz = z;
	for (; lz < 0x2C; lz++) {
		ColResult cr;
		collision_test(&wk->world, lx, ly, lz, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
		if (cr.zf) { wk->x1 = lx; wk->y1 = ly; wk->z = lz; return; }
	}
	walker_find_free(wk, lx, ly);
}

static void walker_tick(Walker *wk, RoomView *rv, int keys) {
	if (rv->restarting) return;
	roomview_tick(rv);
	wk->tick++;
	if (wk->world_key != room_key(rv)) {
		walker_world(wk, rv);
		walker_place_default(wk, rv);
	}
	if (rv->vm.go.pending) {
		rv->vm.go.pending = 0;
		walker_teleport(wk, rv, rv->vm.go.room, rv->vm.go.x, rv->vm.go.y, rv->vm.go.z);
		return;
	}
	if (rv->vm.next_level && rv->ok) {                  /* op 51: the next level; money, score and energy carry over */
		uint32_t money = rv->vm.money, score = rv->vm.score;
		int energy = rv->vm.energy, nl = rv->vm.next_level, tata = rv->use_tata;
		roomview_unload(rv);
		rv->level = nl > (tata ? 4 : 5) ? 1 : nl; rv->use_tata = tata; rv->room = 0;
		wk->world_key = 0; wk->placed = 0;
		rv->restarting = 1;
		rv->carry_pending = 1; rv->carry_money = money; rv->carry_score = score; rv->carry_energy = energy;
		return;
	}
	if (rv->dead_timer == 0 && rv->vm.energy <= 0 && rv->ok && rv->vm.energy_max > 0) {   /* death (1000:81C8): the gods' message, then the game starts over */
		rv->dead_timer = 150;
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 60;
		roomview_flush_messages(rv);
	}
	if (rv->dead_timer > 0) {
		if ((wk->tick & 1) == 0 && --rv->dead_timer == 0) {
			int lv = rv->level, tata = rv->use_tata;
			roomview_unload(rv);
			rv->level = lv; rv->use_tata = tata; rv->room = 0;
			wk->world_key = 0; wk->placed = 0;
			rv->restarting = 1;
		}
		return;
	}
	if (wk->tick & 1)
		return;
	{
		int end = (keys & 0x80) != 0, edge = end && !rv->end_prev;
		rv->end_prev = end;
		if (rv->touch_age < 1000) rv->touch_age++;
		if (rv->menu_open) { menu_step(rv, keys, edge); return; }
		if (edge && rv->nverbs && rv->touch_age < 12 && !(keys & 15)) {   /* End tapped right after touching an object (menu2 trace) */
			rv->menu_open = 1; rv->menu_sel = 0; rv->menu_delay = 0; rv->menu_released = 0;
			return;
		}
	}
	int dx = 0, dy = 0, dir = -1;
	if (keys & 8) { dir = DIR_RIGHT; dx = 1; }
	else if (keys & 4) { dir = DIR_LEFT; dx = -1; }
	else if (keys & 2) { dir = DIR_DOWN; dy = 1; }
	else if (keys & 1) { dir = DIR_UP; dy = -1; }
	if (dir < 0) { wk->moving = 0; wk->anim = 0; return; }
	wk->dir = dir;
	wk->moving = 1;
	wk->world.dirbits = (keys & 3) ? 1 : 0;
	int gw, gh, gx, gy;
	if (!roomview_geom(rv, &gw, &gh, &gx, &gy))
		return;
	ColResult cr;
	collision_test(&wk->world, wk->x1 + dx, wk->y1 + dy, wk->z, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
	if (cr.count == 0xFE) {
		int t = door_target(rv, cr.exit_id);
		if (t >= 0) walker_enter(wk, rv, t, cr.exit_id);
		return;
	}
	if (!cr.zf && cr.count != 0xFF && cr.count != 0xFE)
		walker_touch(wk, rv, &cr, keys);
	static PushObj po[ROOMOBJS_MAX];
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		po[i].x1 = o->x1; po[i].y1 = o->y1; po[i].z0 = o->z0;
		po[i].xs = o->xs; po[i].ys = o->ys; po[i].zs = o->zs; po[i].weight = o->weight;
	}
	int moved[PUSH_MAX], nm;
	int st = push_move(&wk->world, po, wk->x1, wk->y1, wk->z, PL_XS, PL_YS, PL_ZS, -1, dx, dy, 50, moved, &nm);
	if (st == PUSH_MOVED || st == PUSH_FREE) {
		wk->x1 += dx; wk->y1 += dy;
		for (int k = 0; k < nm; k++) {
			RoomObj *o = &rv->ro.o[moved[k]];
			o->x1 += dx; o->y1 += dy;
			roomobj_moved(o, gx, gy);
		}
		if (nm) walker_sync_boxes(wk, rv);
	}
	if ((wk->tick / 2) % 3 == 0) wk->anim = (wk->anim + 1) & 3;
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
	g_plbox.on = g_walk.hdr8_ok && g_walk.world_key == room_key(rv);
	g_plbox.x1 = g_walk.x1; g_plbox.y1 = g_walk.y1; g_plbox.z0 = g_walk.z; g_plbox.z1 = g_walk.z + 30 - 1;
	int rc = roomview_draw(rv, g, scr);
	g_plbox.on = 0;
	rv->scroll_x = sx; rv->scroll_y = sy;
	if (rc != 0) return -1;
	int w, h, gx, gy;
	if (!roomview_geom(rv, &w, &h, &gx, &gy) || !g_walk.hdr8_ok) return 0;
	if (g_walk.world_key != room_key(rv)) { walker_world(&g_walk, rv); walker_place_default(&g_walk, rv); }
	int px, py;
	object_project(g_walk.x1, g_walk.y1, g_walk.z, g_walk.hdr8, gx, gy, &px, &py);
	int f = g_walk.moving ? walk_frames[g_walk.dir][g_walk.anim] : idle_frame[g_walk.dir];
	const Sprite *sp = &g_walk.set.sprites[f];
	blit_sprite(scr, sp, px + sp->hot_x, py + sp->hot_y, 1);
	static uint8_t front[VIEW_W * VIEW_H];
	memset(front, TILE_TRANSPARENT, sizeof(front));
	roomobjs_draw_rel(&rv->ro, front, 1, g_walk.x1, g_walk.y1, g_walk.z, g_walk.z + PL_ZS - 1, 1);
	for (int i = 0; i < VIEW_W * VIEW_H; i++)
		if (front[i] != TILE_TRANSPARENT)
			scr->pixels[i] = front[i];
	panel_draw(g, scr, rv);
	snprintf(rv->info, sizeof(rv->info), "walk: %s level %d room %d - arrows move along the room axes, exits change room",
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
	int game_mode = 0;           /* --scene game: title scroll, then Space/Return starts level 1 in the walk scene */
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
			if (!strcmp(argv[i], "game")) game_mode = 1;
			scene = game_mode ? SCENE_INTRO : !strcmp(argv[i], "objects") ? SCENE_OBJECTS : !strcmp(argv[i], "intro") ? SCENE_INTRO : !strcmp(argv[i], "walk") ? SCENE_WALK : !strcmp(argv[i], "tiles") ? SCENE_TILES : !strcmp(argv[i], "room") ? SCENE_ROOM : !strcmp(argv[i], "loading") ? SCENE_LOADING : !strcmp(argv[i], "sprites") ? SCENE_SPRITES
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
				else if (game_mode && (k == SDLK_SPACE || k == SDLK_RETURN)) {
					if (scene == SCENE_INTRO) { scene = SCENE_WALK; g_room.level = 1; g_room.room = 0; g_room.use_tata = 0; }
					else changed = 0;
				}
				else if (scene == SCENE_WALK && k == SDLK_i) { g_room.show_status ^= 1; changed = 0; }
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
			           (ks[SDL_SCANCODE_RIGHT] ? 8 : 0) | (ks[SDL_SCANCODE_END] ? 0x80 : 0);
			while (now - last_ms >= 20) {            /* the game runs at ~50 Hz */
				walker_tick(&g_walk, &g_room, keys);
				last_ms += 20;
			}
			if (sim && frames == 0) {                 /* headless: --sim right:30,down:10 */
				for (const char *c = sim; *c;) {
					int bits = !strncmp(c, "rend", 4) ? 0x88 : !strncmp(c, "right", 5) ? 8 : !strncmp(c, "left", 4) ? 4 : !strncmp(c, "down", 4) ? 2 : !strncmp(c, "up", 2) ? 1 : !strncmp(c, "end", 3) ? 0x80 : 0;
					const char *colon = strchr(c, ':');
					if (!colon) break;
					if (!strncmp(c, "info", 4)) g_room.show_status ^= 1;
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
