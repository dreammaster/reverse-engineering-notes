/*
 * Cadaver / Cadaver: The Payoff - reimplementation front end (SDL2).
 *
 *   cadaver [-g <game dir>] [--scene game|logo|loading|sprites|sprites2|room|tiles|walk|intro|objects] [--level N --room R --tata --scroll Y --sim dir:ticks,...] [--screenshot out.bmp]
 *
 * Current state: `--scene game` plays levels of either game in the walk scene: movement, jumping, pushing, scripts and timers, creatures (TAD routines),
 * verbs, chests, rucksack, map, status dialog, level progression, death, quick save/load and PC-speaker sound. Not done: see docs/roadmap.md.
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
#include "engine/ruckgrid.h"
#include "engine/describe.h"
#include "engine/roomexits.h"
#include "engine/panel.h"
#include "engine/push.h"
#include "engine/acl.h"
#include "engine/lng.h"
#include "engine/message.h"
#include "engine/tad.h"
#include "engine/beeper.h"

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
	struct { int active, id, timer, dot; char txt[200]; } msgs[4];   /* txt: a composed text (id = -2) */   /* message scrolls (slots 0..3) */
	uint8_t verbs[VERB_MAX];    /* verb icons of the touched / carried object (two pages of six) */
	int nverbs;
	int energy_max_cfg;        /* from LEVn.MP1 */
	int slow_phase, tick_dbg;
	int fire_cool;                        /* ds:629: the cooldown after a spell / weapon shot (A[6] / A[3]), counted down once per game step (1000:02BD) */
	int cast_target;                      /* ds:6A8 / ds:DA..E0: the touched thing a spell is cast on through verb 15 (0 = none) */
	int save_price;                       /* ds:246: the price of the next saved position */
	uint16_t spell_str[100], potion_str[100]; int spell_tbl_ok;   /* LEVn.MP1: LNG string ids of the spell / potion names (the tables ds:44 / ds:46) */
	int touch_age, menu_open, menu_sel, menu_delay, menu_released, touch_thing, end_prev;   /* verb menu (End held / pressed while touching an object) */
	int carry_pending, carry_energy; uint32_t carry_money, carry_score;   /* stats carried into the next level */
	int room_snd[8], room_snd_thing[8], nroom_snd, snd_key; /* sounds of the room's creatures (stopped when the room changes) */
	int ruck_target;           /* thing touched when the rucksack was opened (insert / give target), -1 = none */
	int gx, grow, gvis, gscroll;   /* the grid cursor of 1000:125CF: column ds:6AF, absolute row ds:6B0, visible row ds:6B1, first visible row ds:6B6 */
	int grid_open;             /* the rucksack strip shown with a look at a carried thing: all carried things, the current one framed in green (emulator, 2 things) */
	int look_pic;              /* thing+1 whose picture is shown next to the look message (rucksack look), 0 = none */
	int map_magic;             /* spell 4 (TAD routine 0974, ds:6C9 = 81h): the map shows every room not flagged 80h in its record byte 17h */
	int map_open, map_ox, map_oy, map_zoom, map_prev;   /* F1: the map of the rooms entered so far (1000:369F) */
	struct { int thing, zi, hi, dirbits, fall, weightless; } fly[8];   /* thrown things in flight (1000:16D3A: per-step z and horizontal path tables) */
	int nfly;
	int h_prev;
	int h_item;                 /* ds:58A: the thing H put away last (the only thing H can hold again; 0 until H has put something away) */
	uint8_t loose[ACL_THINGS];  /* things dropped or thrown by the player: gravity applies */
	int held;                  /* thing held (verb 13), 0 = none */
	int ruck_open, ruck_sel, ruck_delay, last_item, space_prev, ret_prev;   /* rucksack view (Space / Return while carrying things) */
	int push_snd;              /* looping sound of the object being pushed (0 = none) */
	int restarting;            /* level torn down after a death; the next draw reloads it */
	int dead_timer;            /* > 0: the player died (energy 0), counts game steps until the level restarts */
	RankTable ranks; int ranks_ok; unsigned six_fa;   /* status dialog data (MP1) */
	int show_status;           /* 'I': the character status dialog */
	int ending;                /* the game is over (TAD service 3Ah): the status box stays up and a key leaves */
	int creature_tick;
	unsigned clock_ticks; int day;   /* the game clock: 50 Hz ticks; one day = 30 minutes (1000:1514: ds:5B7/5B5/5B4/5B2) */
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
/* ---- sound: BEEPER.BIN on the interpreter (engine/beeper.c); ids as the game's 1000:156C / 1000:1613 take them ---- */
static const char *g_lang = "eng";     /* --lang eng|fre|ger: the language file ENGn.LNG / FREn.LNG / GERn.LNG */
static int g_four_dir;                /* F4: four-direction movement (diagonals ignored) */
static const char *g_dbg_zones, *g_dbg_creat, *g_dbg_shot, *g_dbg_vars;   /* debug environment, read once in main (getenv per step is slow) */
static int g_paused;                  /* Pause key in the walk scene */
static int g_quit_req;                /* the game ended and a key was pressed: leave the main loop */
static const char *g_sound_opt;       /* --sound adlib|beeper|sblaster|roland */
static int g_payoff;                  /* Cadaver 2 is installed (The Payoff follows Cadaver's last level) */
static Beeper g_snd;
static int g_snd_ok, g_snd_log;
static SDL_AudioDeviceID g_adev;
static void snd_lock(int on) { if (g_adev) { if (on) SDL_LockAudioDevice(g_adev); else SDL_UnlockAudioDevice(g_adev); } }
static void snd_play(int id) {
	if (g_snd_log) fprintf(stderr, "SND play %d\n", id);
	if (!g_snd_ok) return;
	snd_lock(1); beeper_play(&g_snd, id); snd_lock(0);
}
static void snd_stop(int id) {
	if (g_snd_log) fprintf(stderr, "SND stop %d\n", id);
	if (!g_snd_ok) return;
	snd_lock(1); beeper_stop(&g_snd, id); snd_lock(0);
}
static void snd_music(int on) {
	if (!g_snd_ok) return;
	snd_lock(1); beeper_music(&g_snd, on); snd_lock(0);
}
/* level change / restart (1000:15EB): silence everything */
static void snd_silence(void) {
	if (!g_snd_ok) return;
	snd_lock(1); beeper_silence(&g_snd); snd_lock(0);
}
static void snd_callback(void *u, Uint8 *stream, int len) { (void)u; beeper_render(&g_snd, (int16_t *)stream, len / 2); }
static void game_sound(int id, void *u) { (void)u; snd_play(id); }
static void game_sound_stop(int id, void *u) { (void)u; snd_stop(id); }
/* ds:FED9: sound class of a pushed object (sprite header byte 1Dh) -> looping sound id */
static const uint8_t push_sound[19] = {0x39, 0x10, 0x39, 0x33, 0x34, 0x39, 0x33, 0x39, 0x39, 0x39, 0x33, 0x39, 0x33, 0x39, 0x39, 0x34, 0x34, 0x34, 0x39};

static int game_rand(int lo, int hi, void *u) {
	(void)u;
	static unsigned s = 12345;
	s = s * 1103515245u + 12345u;
	return hi <= lo ? lo : lo + (int)((s >> 16) % (unsigned)(hi - lo + 1));
}
static void roomview_game_step(RoomView *rv);
static void walk_refresh_world(RoomView *rv);
static void roomview_ensure_objs(RoomView *rv, int gx, int gy);
static void roomview_flush_messages(RoomView *rv);
static int rv_changed(const RoomView *rv);
static void roomview_tick(RoomView *rv) {
	if (rv->ok && ++rv->clock_ticks % (50u * 60u * 30u) == 0) {      /* the timer interrupt: 50 ticks = 1 s, 60 s = 1 min, 30 min = a day -> message 28 "A DAY PASSES ..." */
		rv->day++;
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 28;
	}
	if (rv->ok && rv->ro_key && (++rv->ro_ticks & 1) == 0) {
		roomobjs_step(&rv->ro);
		for (int i = 0; i < rv->ro.n; i++) {                       /* animation commands F6 / F5 n: A[8 + n] -= / += 1 */
			RoomObj *o = &rv->ro.o[i];
			for (int k = 0; k < o->an.ncnt; k++)
				if (o->an.cntn[k] >= 0 && o->an.cntn[k] < 8) rv->vm.tail[o->thing][8 + o->an.cntn[k]] = (uint8_t)(rv->vm.tail[o->thing][8 + o->an.cntn[k]] + o->an.cntd[k]);
			o->an.ncnt = 0;
		}
		for (int i = 0; i < rv->ro.n; i++) {                       /* animation command F7 n: the thing's TAD action routine n (1000:3D17 queues, 5F6E runs) */
			RoomObj *o = &rv->ro.o[i];
			for (int k = 0; k < o->an.nact; k++)
				if (rv->tad.loaded) tad_run_action(&rv->tad, &rv->vm, o->thing, o->an.act[k], &o->an);
			o->an.nact = 0;
		}
		for (int k = 0; k < rv->vm.nspawn; k++) {                  /* shots fired by TAD routines (service 22h) */
			acl_spawn_shot(&rv->vm, rv->vm.spawn[k].id, rv->vm.spawn[k].from, rv->vm.spawn[k].vx, rv->vm.spawn[k].vy);
			if (g_snd_log) fprintf(stderr, "SHOT thing %d from %d dir %d\n", rv->vm.spawn[k].id, rv->vm.spawn[k].from, rv->vm.spawn[k].dir);
		}
		if (rv->vm.nspawn) rv->pending_rebuild = 1;
		rv->vm.nspawn = 0;
		for (int k = 0; k < rv->vm.nact; k++)                       /* op 42h */
			if (rv->tad.loaded) {
				AnimState *an = NULL;
				for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == rv->vm.act_thing[k]) an = &rv->ro.o[i].an;
				tad_run_action(&rv->tad, &rv->vm, rv->vm.act_thing[k], rv->vm.act_arg[k], an);
			}
		rv->vm.nact = 0;
		for (int k = 0; k < rv->vm.nresume; k++)                    /* op 03h */
			for (int i = 0; i < rv->ro.n; i++)
				if (rv->ro.o[i].thing == rv->vm.resume[k] && rv->ro.o[i].an.delay == 0xFE) { rv->ro.o[i].an.delay = 0; rv->ro.o[i].an.pos++; }
		rv->vm.nresume = 0;
		if (g_snd_log) for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == 467) { static int pp = -9, pd = -9, pf = -9; const AnimState *a = &rv->ro.o[i].an; if (a->pos != pp || a->delay != pd || a->frame != pf) { fprintf(stderr, "A467 tick %u pos %d delay %d frame %d cnt %d cad %d\n", rv->clock_ticks, a->pos, a->delay, a->frame, a->cnt, a->cad); pp = a->pos; pd = a->delay; pf = a->frame; } }
		for (int i = 0; i < rv->ro.n; i++) {                       /* animation command F8 n: event 13 (argument n) for the thing (1000:3CE7) */
			RoomObj *o = &rv->ro.o[i];
			while (o->an.nev > 0) {
				int arg = o->an.ev[0];
				memmove(o->an.ev, o->an.ev + 1, (size_t)(o->an.nev - 1));
				o->an.nev--;
				rv->vm.queue_n = 0;
				acl_thing_event(&rv->vm, &rv->mp2, o->thing, 13, arg);
				for (int q = 0; q < rv->vm.queue_n; q++)
					acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
				rv->pending_rebuild |= rv_changed(rv);
				roomview_flush_messages(rv);
			}
		}
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
static int spell_sem(const RoomView *rv, int n);
static const ObjSprite *thing_sprite(const RoomView *rv, int t);
static int thing_description(const RoomView *rv, int t, const ObjSprite *lsp, char *tx, size_t cap);
static void ruck_picture(uint8_t *px, const ObjSprite *sp);
static void look_picture(uint8_t *px, const ObjSprite *sp);
static void grid_draw(uint8_t *px, const RoomView *rv);
static int ruck_list(const RoomView *rv, int *out, int max);
static void map_draw(uint8_t *px, const RoomView *rv);
static void map_open_magic(RoomView *rv);
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
		status_dialog_draw_at(scr->pixels, rv->ending ? 130 : 64, rv->ending ? 108 : 10, g_tiles, g_tiles_len, g_font3, g_font3_len, rv->names.count > 13 ? rv->names.strings[13] : "",
		                   rv->vm.energy, rv->vm.energy_max, rv->vm.money, rv->vm.score, rv->level, complete_percent(vis, rv->six_fa, w5),
		                   rid < rv->names.count ? rv->names.strings[rid] : "", msg_rnd, NULL);
	}
	if (g_icons && rv->nverbs) {
		int pg = (rv->menu_open || rv->ruck_open) ? rv->menu_sel / VERB_SLOTS : 0, cnt = rv->nverbs - pg * VERB_SLOTS;   /* the second page of six */
		verbs_draw(scr->pixels, rv->verbs + pg * VERB_SLOTS, cnt > VERB_SLOTS ? VERB_SLOTS : cnt, g_icons, g_icons_len);
		if (rv->menu_open && !rv->grid_open)
			verbs_highlight(scr->pixels, rv->menu_sel % VERB_SLOTS);
	}
	if (g_font && rv->names_ok && rv->room >= 0 && rv->room < rv->mp2.sec[0].count && rv->mp2.sec[0].rec[rv->room]) {
		char nm[48];
		status_room_name(rv->mp2.sec[0].rec[rv->room], (size_t)rv->mp2.sec[0].len[rv->room], rv->name_tbl, rv->names.strings,
		                 rv->names.count, nm, sizeof(nm));
		if (!(rv->ruck_open && !rv->grid_open)) status_draw(scr->pixels, g_font, g_font_len, nm, rv->day + 1);
		int pic = 0;
		for (int sl = 0; sl < 4; sl++) pic |= rv->msgs[sl].active;
		pic = (pic && rv->look_pic) || rv->grid_open;
		if (rv->grid_open && g_tiles) grid_draw(scr->pixels, rv);
		else if (pic && g_tiles) look_picture(scr->pixels, thing_sprite(rv, rv->look_pic - 1));
		for (int sl = 0; sl < 4 && g_tiles && g_font3; sl++)
			if (rv->msgs[sl].active && ((rv->msgs[sl].id >= 0 && rv->msgs[sl].id < rv->names.count) || rv->msgs[sl].id == -2)) {
				char tx[256];
				if (rv->msgs[sl].id == -2) snprintf(tx, sizeof(tx), "%s", rv->msgs[sl].txt);
				else snprintf(tx, sizeof(tx), "%s%s", rv->names.strings[rv->msgs[sl].id], rv->msgs[sl].dot ? "." : "");
				if (pic) message_draw_at(scr->pixels, tx, 208, 20 + 16 * sl, g_tiles, g_tiles_len, g_font3, g_font3_len, msg_rnd, NULL);
				else message_draw(scr->pixels, tx, sl, g_tiles, g_tiles_len, g_font3, g_font3_len, msg_rnd, NULL);
			}
		if (rv->ruck_open && !rv->grid_open && rv->label_id > 0 && rv->label_id < rv->names.count)       /* the item's name replaces the room name */
			status_draw_centred(scr->pixels, 202, 186, 108, rv->names.strings[rv->label_id], g_font, g_font_len);
		else if (rv->label_id > 0 && rv->label_id < rv->names.count) {
			char lb[16];
			snprintf(lb, sizeof(lb), "%.12s", rv->names.strings[rv->label_id]);
			status_draw_centred(scr->pixels, 229, 172, 84, lb, g_font, g_font_len);
		}
	}
	if (rv->ruck_open && !rv->grid_open) ruck_picture(scr->pixels, thing_sprite(rv, rv->touch_thing));
	if (rv->map_open) map_draw(scr->pixels, rv);
}

/* while the walk demo runs, objects in front of the player are drawn after it (see walk_draw) */
static struct { int on, x1, y1, z0, z1; } g_plbox;

static void roomview_unload(RoomView *rv) {
	rv->ro_key = 0;
	rv->push_snd = 0;
	rv->nfly = 0;
	rv->nroom_snd = 0;
	rv->snd_key = 0;
	snd_silence();
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

static void game_load_apply(RoomView *rv);
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
		char lp[64];
		snprintf(lp, sizeof(lp), "%s\\?\\%s?.lng", rv->use_tata ? "tata" : "data", g_lang);
		if (gamefiles_read_mjm(g, lp, rv->level, &lg, &ll) == 0) {
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
	if (rc == 0) rv->spell_tbl_ok = describe_load_tables(f[0], fl[0], rv->spell_str, rv->potion_str);   /* the spell and potion name tables of the level's MP1 */
	for (int i = 0; i < 3; i++) free(f[i]);
	if (rc != 0) {
		fprintf(stderr, "level %d: decode error %d\n", rv->level, rc);
		return -1;
	}
	acl_init(&rv->vm, &rv->mp2);
	rv->vm.rand_range = game_rand;
	rv->vm.clone_ok = 1;
	for (int t = 0; t < ACL_THINGS; t++) {                    /* sprite box sizes for op 38h */
		int len;
		const uint8_t *r = mp2_thing(&rv->mp2, t, &len);
		const ObjSprite *sp = r && len >= 8 ? objsprites_find(&rv->objs, (r[6] << 8) | r[7]) : NULL;
		if (sp) { rv->vm.size[t][0] = sp->header[17]; rv->vm.size[t][1] = sp->header[19]; rv->vm.size[t][2] = sp->header[21]; }
	}
	rv->vm.sound = game_sound;
	rv->vm.sound_stop = game_sound_stop;
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
	if (getenv("CAD_CARRY")) { int t = 0, a0 = -1, a1 = -1; sscanf(getenv("CAD_CARRY"), "%d:%d:%d", &t, &a0, &a1); if (t > 0 && t < ACL_THINGS) { rv->vm.present[t] = 1; rv->vm.room[t] = 0xFF; rv->vm.flags[t] &= (uint8_t)~0x80; rv->last_item = t; rv->h_item = t; if (a0 >= 0) rv->vm.tail[t][0] = (uint8_t)a0; if (a1 >= 0) rv->vm.tail[t][1] = (uint8_t)a1; } }   /* debugging aid: carry this thing from the start (optionally with A[0], A[1]) */
	if (getenv("CAD_SPELLS")) for (int t = 0; t < ACL_THINGS; t++) { const ObjSprite *sp = thing_sprite(rv, t); if (!sp || !rv->vm.present[t] || (sp->header[0x16] != 1 && sp->header[0x16] != 2 && sp->header[0x16] != 4)) continue; const uint8_t *a = rv->vm.tail[t];
		fprintf(stderr, "SPELLTHING %d class %d A %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", t, sp->header[0x16], a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]); }
	if (getenv("CAD_CARRYN")) { int n = atoi(getenv("CAD_CARRYN")); for (int t = 1; t < ACL_THINGS && n > 0; t++) { const ObjSprite *sp = thing_sprite(rv, t); if (!rv->vm.present[t] || !sp || !(sp->header[0x17] || sp->header[0x16] == 1 || sp->header[0x16] == 2) || sp->nframes < 1) continue; rv->vm.room[t] = 0xFF; rv->vm.flags[t] &= (uint8_t)~0x80; rv->last_item = t; rv->h_item = t; n--; } }   /* debugging aid: carry this many things from the start */
	if (getenv("CAD_MOVE")) { int t = 0, x = 0, y = 0, rm = -1; if (sscanf(getenv("CAD_MOVE"), "%d:%d:%d:%d", &t, &x, &y, &rm) >= 3 && t > 0 && t < ACL_THINGS) { rv->vm.pos[3 * t] = (uint8_t)x; rv->vm.pos[3 * t + 1] = (uint8_t)y; if (rm >= 0) rv->vm.room[t] = (uint8_t)rm; } }   /* debugging aid: put a thing at x, y (and into a room) */
	if (getenv("CAD_SCRIPT")) { int t = atoi(getenv("CAD_SCRIPT")); const ObjSprite *q = thing_sprite(rv, t); if (q) { int n = anim_script_len(q); fprintf(stderr, "SCRIPT thing %d (%d words):", t, n); for (int i = 0; i < n; i++) { int cmd, arg; anim_script_word(q, i, &cmd, &arg); fprintf(stderr, " %02x%02x", cmd, arg); } fprintf(stderr, "\n"); } }       /* debugging aid: the animation script of a thing */
	if (getenv("CAD_FINDF7")) { int want = atoi(getenv("CAD_FINDF7")); for (int t = 1; t < ACL_THINGS; t++) { const ObjSprite *q = thing_sprite(rv, t); if (!rv->vm.present[t] || !q) continue; int n = anim_script_len(q); for (int i = 0; i < n; i++) { int cmd, arg; anim_script_word(q, i, &cmd, &arg); if (cmd == 0xF7 && (want < 0 || arg == want)) { fprintf(stderr, "F7 %d: thing %d class %02x room %d tail1 %d flags %02x\n", arg, t, q->header[22], rv->vm.room[t], rv->vm.tail[t][1], rv->vm.flags[t]); } } } }       /* debugging aid: the things whose animation script runs action routine n (command F7 n) */
	if (getenv("CAD_FINDBEH")) { int b = atoi(getenv("CAD_FINDBEH")); for (int t = 1; t < ACL_THINGS; t++) if (rv->vm.present[t] && rv->vm.tail[t][1] == b) { int len; const uint8_t *r = mp2_thing(&rv->mp2, t, &len); fprintf(stderr, "BEH thing %d room %d pos %d,%d,%d flags %02x nev %d\n", t, rv->vm.room[t], rv->vm.pos[3*t], rv->vm.pos[3*t+1], rv->vm.pos[3*t+2], rv->vm.flags[t], r ? r[11] : -1); } }       /* debugging aid: the things whose behaviour byte A[1] is n */
	if (getenv("CAD_CLASS")) { int t = atoi(getenv("CAD_CLASS")); const ObjSprite *sp = thing_sprite(rv, t); if (sp) { fprintf(stderr, "CLASS header:"); for (int q = 0; q < 32; q++) fprintf(stderr, " %02x", sp->header[q]); fprintf(stderr, "\n"); } fprintf(stderr, "CLASS %d: %d mode %d A %02x %02x %02x %02x %02x %02x %02x %02x flags %02x room %d\n", t, sp ? sp->header[0x16] : -1, rv->vm.mstate[t][0], rv->vm.tail[t][0], rv->vm.tail[t][1], rv->vm.tail[t][2], rv->vm.tail[t][3], rv->vm.tail[t][4], rv->vm.tail[t][5], rv->vm.tail[t][6], rv->vm.tail[t][7], rv->vm.flags[t], rv->vm.room[t]); fprintf(stderr, "  B"); for (int i = 0; i < 10; i++) fprintf(stderr, " %02x", rv->vm.mstate[t][i]); fprintf(stderr, " pos %d,%d,%d\n", rv->vm.pos[3*t], rv->vm.pos[3*t+1], rv->vm.pos[3*t+2]); }
	if (getenv("CAD_LOOKTEXT")) { int t = atoi(getenv("CAD_LOOKTEXT")); char tx[200] = ""; const ObjSprite *sp = thing_sprite(rv, t);   /* debugging aid: print the class-specific description of a thing */
		fprintf(stderr, "LOOK %d class %d: %s\n", t, sp ? sp->header[0x16] : -1, sp && thing_description(rv, t, sp, tx, sizeof(tx)) ? tx : "(none)"); }
	if (getenv("CAD_ENERGY")) rv->vm.energy = atoi(getenv("CAD_ENERGY"));     /* debugging aid: start with this energy */
	memset(rv->visited, 0, sizeof(rv->visited));
	if (rv->carry_pending) {
		rv->carry_pending = 0;
		rv->vm.money = rv->carry_money; rv->vm.score = rv->carry_score;
		rv->vm.energy = rv->carry_energy > rv->vm.energy_max ? rv->vm.energy_max : rv->carry_energy;
	}
	game_load_apply(rv);
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

static RoomView g_room = {.level = 1};

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
	int face;                  /* ds:608: the movement key bits (1 up 2 down 4 left 8 right) of the last move, diagonals included */
	int world_key, placed;
	int jumping, jstep, jhold, jcool, fall, giant;   /* jump (fire held, nothing touched): z profile below */
	int grab, grab_rev;        /* verb 10 (1000:336A, ds:74): the grabbed thing + 1 (0 = none); grab_rev: the walking frames are those of the opposite facing played backwards (script table ds:FA93) */
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
			if (rv->msgs[i].active && rv->msgs[i].id == (rv->vm.msg[k] & 0x7FFF)) { sl = i; break; }
		if (sl >= 0) {
			rv->msgs[sl].timer = MSG_STEPS;
			continue;
		}
		for (int i = 0; i < 4; i++) {
			if (!rv->msgs[i].active) { sl = i; break; }
			if (rv->msgs[i].timer < best) { best = rv->msgs[i].timer; sl = i; }
		}
		rv->msgs[sl].active = 1;
		rv->msgs[sl].id = rv->vm.msg[k] & 0x7FFF;
		rv->msgs[sl].dot = (rv->vm.msg[k] & 0x8000) != 0;
		rv->msgs[sl].timer = MSG_STEPS;
	}
	rv->vm.msg_n = 0;
}

/* show a composed text as a message scroll (the look descriptions of 1000:84D9) */
static void msg_show_text(RoomView *rv, const char *text) {
	int sl = -1, best = 1 << 30;
	for (int i = 0; i < 4; i++) {
		if (!rv->msgs[i].active) { sl = i; break; }
		if (rv->msgs[i].timer < best) { best = rv->msgs[i].timer; sl = i; }
	}
	rv->msgs[sl].active = 1; rv->msgs[sl].id = -2; rv->msgs[sl].dot = 0; rv->msgs[sl].timer = MSG_STEPS;
	snprintf(rv->msgs[sl].txt, sizeof(rv->msgs[sl].txt), "%s", text);
}
static const char *lng_str(const RoomView *rv, int id) { return id >= 0 && id < rv->names.count ? rv->names.strings[id] : ""; }

/* 1000:84D9: the class-specific part of a look text (emulator-verified with the real routine on level 1's records:
   spell items, potions, weapons and ammunition); returns 0 when the class has none */
static int thing_description(const RoomView *rv, int t, const ObjSprite *lsp, char *tx, size_t cap) {
	if (!lsp || t < 0 || t >= ACL_THINGS || !rv->spell_tbl_ok) return 0;
	const uint8_t *a = rv->vm.tail[t];
	int cls = lsp->header[0x16], ammo = (a[0] << 8) | a[1], ammo_name = 0;
	const uint8_t *ammo_a = NULL;
	if (cls == 4 && ammo < ACL_THINGS) {
		ammo_a = rv->vm.tail[ammo];
		const ObjSprite *asp = thing_sprite(rv, ammo);
		ammo_name = asp ? (asp->header[10] << 8) | asp->header[11] : 0;
	}
	return item_description(&rv->names, rv->spell_str, rv->potion_str, cls, a, ammo_a, ammo_name, tx, cap);   /* engine/describe.c, verified against 1000:84D9 */
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

/* Room zones (1000:11EB5 -> 1000:2022 .. 2108): a room record whose byte 17h has bit 1 set carries, at rec[rec[0]], a box count n and then n boxes (x0, y0, x1, y1, z0, z1 as signed bytes).
 * Every 17 game steps each object of the room is tested against the boxes n .. 1 with the collision box it keeps (x1, y1, x0, y0, zt, zb); an overlap raises event 10 (0Ah) for the
 * thing with the box number as argument (level 3 / 4 puzzles: things pushed onto plates; the handlers test room + box). The original tests the things whose engine entry flag byte
 * 27h (= sprite header byte 0Ch, checked in the emulator for every slot of level 1 room 0) has bit 0 (non-solid) and, unless the room has bit 10h, bit 2 (11FDF .. 11FFD). */
static void roomview_zone_events(RoomView *rv) {
	if (rv->room < 0 || rv->room >= 128 || !rv->ro_key) return;
	const uint8_t *r = rv->mp2.sec[0].rec[rv->room];
	int len = rv->mp2.sec[0].len[rv->room];
	if (!r || len <= 0x20 || !(r[0x17] & 2) || r[0] + 2 >= len) return;
	int n = r[r[0]];
	if (n < 1 || n > 4 || r[0] + 2 + 6 * n > len) return;
	const int8_t *boxes = (const int8_t *)(r + r[0] + 2);
	int all = (r[0x17] & 0x10) != 0;                                  /* room flag 10h: every thing is tracked and the room's own events 15 / 17 are raised */
	const Walker *wk = &g_walk;
	for (int b = n; b >= 1 && all && wk->world_key == room_key(rv); b--) {          /* the player (thing 0): room event 15 (0xC00F), argument = the box number */
		const int8_t *bx = boxes + 6 * (b - 1);
		int dl = wk->x1, dh = wk->y1, cl = wk->x1 - PL_XS + 1, bl = wk->y1 - PL_YS + 1, ch = wk->z, bh = wk->z + PL_ZS - 1;
		if (!zone_overlap(bx, (int8_t)dl, (int8_t)dh, (int8_t)cl, (int8_t)bl, (int8_t)bh, (int8_t)ch)) continue;
		if (g_dbg_zones) fprintf(stderr, "ZONE player box %d room %d\n", b, rv->room);
		int keep = rv->vm.cur; rv->vm.cur = 0;
		acl_room_event(&rv->vm, &rv->mp2, rv->room, 15, b);
		rv->vm.cur = keep;
	}
	for (int i = 0; i < rv->ro.n && i < ROOMOBJS_MAX; i++) {
		const RoomObj *o = &rv->ro.o[i];
		int t = o->thing;
		if (t <= 0 || t >= ACL_THINGS || (rv->vm.flags[t] & 0x80)) continue;
		if (!(o->sp->header[12] & 1) || !(all || (o->sp->header[12] & 4))) continue;      /* 11FDF: the engine entry's flag byte 27h (= sprite header byte 0Ch) needs bit 0 (non-solid) and, unless the room has bit 10h, bit 2 */
		int dl = (int8_t)o->cx1, dh = (int8_t)o->cy1, cl = (int8_t)o->cx0, bl = (int8_t)o->cy0, bh = (int8_t)o->czt, ch = (int8_t)o->czb;
		for (int b = n; b >= 1; b--) {
			if (!zone_overlap(boxes + 6 * (b - 1), dl, dh, cl, bl, bh, ch)) continue;
			if (g_dbg_zones) fprintf(stderr, "ZONE thing %d box %d (x %d..%d y %d..%d z %d..%d) room %d\n", t, b, cl, dl, bl, dh, ch, bh, rv->room);
			rv->vm.queue_n = 0;
			acl_thing_event(&rv->vm, &rv->mp2, t, 10, b);
			for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
			if (all) { int keep = rv->vm.cur; rv->vm.cur = t; acl_room_event(&rv->vm, &rv->mp2, rv->room, 17, b); rv->vm.cur = keep; }   /* another thing: room event 17 (0xC011) */
		}
	}
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
}

/* one game step (25 Hz): the room timers of 1000:11EB5 (units of 17 steps) -> events 14 and 20 */
static void roomview_game_step(RoomView *rv) {
	if (rv->fire_cool > 0) rv->fire_cool--;
	if (rv->vm.msg_n) roomview_flush_messages(rv);
	for (int i = 0; i < 4; i++)
		if (rv->msgs[i].active && !rv->ending && --rv->msgs[i].timer <= 0)       /* the ending's scroll stays up until a key */
			rv->msgs[i].active = 0;
	if (rv->room < 0 || rv->room >= 128 || --rv->tm_div > 0)
		return;
	rv->tm_div = 17;
	tad_timer_unit(&rv->tad, &rv->vm);
	roomview_zone_events(rv);
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
	/* 1000:51E2 / 5114: an active object (creature) of the room plays the sound in its sprite header byte 1Eh (FFh and 3Bh = none) while it is in the room */
	if (rv->snd_key != room_key(rv)) {                               /* a new room: the old room's sounds stop */
		for (int i = 0; i < rv->nroom_snd; i++) snd_stop(rv->room_snd[i]);
		rv->nroom_snd = 0;
		rv->snd_key = room_key(rv);
	}
	{                                                                /* things appearing / disappearing (flag 80h) start / stop their sound (the water creature of level 1 surfaces and sinks) */
		int want_t[8], want_s[8], nw = 0;
		for (int i = 0, active = 0; i < rv->ro.n && active < 6; i++) {
			const RoomObj *o = &rv->ro.o[i];
			if (o->sp->header[0x16] != 3 || (rv->vm.tail[o->thing][5] & 0x80) || (rv->vm.flags[o->thing] & 0x80)) continue;
			active++;
			int id = o->sp->header[0x1E];
			if (id == 0xFF || id == 0x3B || nw >= 8) continue;
			want_t[nw] = o->thing; want_s[nw++] = id;
		}
		int keep_t[8], keep_s[8], nk = 0;
		for (int i = 0; i < rv->nroom_snd; i++) {
			int still = 0;
			for (int k = 0; k < nw; k++) if (want_t[k] == rv->room_snd_thing[i]) still = 1;
			if (still) { keep_t[nk] = rv->room_snd_thing[i]; keep_s[nk++] = rv->room_snd[i]; }
			else snd_stop(rv->room_snd[i]);
		}
		for (int k = 0; k < nw; k++) {
			int have = 0;
			for (int i = 0; i < nk; i++) if (keep_t[i] == want_t[k]) have = 1;
			if (!have) { if (g_snd_log) fprintf(stderr, "SNDTHING %d room %d\n", want_t[k], rv->room); snd_play(want_s[k]); keep_t[nk] = want_t[k]; keep_s[nk++] = want_s[k]; }
		}
		for (int i = 0; i < nk; i++) { rv->room_snd_thing[i] = keep_t[i]; rv->room_snd[i] = keep_s[i]; }
		rv->nroom_snd = nk;
	}
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
	if (getenv("CAD_HDRS")) for (int i = 0; i < rv->ro.n; i++) { const ObjSprite *q = rv->ro.o[i].sp; fprintf(stderr, "HDR thing %d h12 %02x w13 %02x size %d,%d,%d class %02x\n", rv->ro.o[i].thing, q->header[12], q->header[13], q->header[17], q->header[19], q->header[21], q->header[22]); }
	if (getenv("CAD_BOXES")) for (int i = 0; i < rv->ro.n; i++) fprintf(stderr, "BOX %d thing %d x %d..%d y %d..%d z %d..%d solid %d\n", i, wk->objs[i].id, wk->objs[i].x0, wk->objs[i].x1, wk->objs[i].y0, wk->objs[i].y1, wk->objs[i].zb, wk->objs[i].zt, wk->objs[i].solid);
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

/* A thing of the player's (class 81h / 84h weapon ammunition, 82h spell projectile) flies into a creature (1000:6F62 loc_17327 / loc_17250). Weapon shots: the creature
 * is spared when its A[5] has the bit of the shot's damage type A[5] or A[6] bit 0 is set, otherwise A[1] of the shot is subtracted from its hit points A[0]; at 0 or
 * below event 23 (death). Spell projectiles run the spell's routine with the creature as the target: spell 0 (085C) subtracts A[1] and kills on underflow. */
static void shot_hits(RoomView *rv, int shot, int victim) {
	if (victim <= 0 || victim >= ACL_THINGS || victim == shot) return;
	const ObjSprite *vs = thing_sprite(rv, victim), *ss = thing_sprite(rv, shot);
	if (!vs || !ss || vs->header[0x16] != 3 || (rv->vm.flags[victim] & 0x80) || (rv->vm.tail[victim][5] & 0x80)) return;
	const uint8_t *sa = rv->vm.tail[shot];
	uint8_t *ca = rv->vm.tail[victim];
	int dead = 0;
	if (ss->header[0x16] == 0x82) {
		if ((sa[0] & 0x7F) != 0) return;
		if (ca[0] < sa[1]) dead = 1;                                  /* 085C: sub [si], al; jae -> the byte wraps, event 17h on the borrow */
		ca[0] = (uint8_t)(ca[0] - sa[1]);
	} else {
		int type = sa[5];
		if ((type < 8 && (ca[5] & (1 << type))) || (ca[6] & 1)) return;
		if ((int8_t)ca[0] > (int8_t)sa[1]) ca[0] = (uint8_t)(ca[0] - sa[1]);   /* 17367: sub dh, al; jg (signed bytes: a power above 127 wraps; verified with the original) */
		else dead = 1;                                                       /* the hit points stay, event 17h */
	}
	if (dead) {
		rv->vm.queue_n = 0;
		acl_thing_event(&rv->vm, &rv->mp2, victim, 23, 0);
		for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
		rv->pending_rebuild |= rv_changed(rv);
		roomview_flush_messages(rv);
	}
}

/* One move attempt of a creature (1000:6F62 -> 1000:116E0): if the target box overlaps the player the creature stays and the player is hit — energy -= A[1] (the
 * creature's tail byte 1, 1000:72EC -> 81C8) and event 9 (argument 0) is raised for the creature; otherwise the game's collision test decides. */
static void creature_move(Walker *wk, RoomView *rv, RoomObj *o, int ddx, int ddy, int gx, int gy, int *moved, int *hit) {
	int tx = o->x1 + ddx, ty = o->y1 + ddy;
	int ox0 = tx - o->xs, oy0 = ty - o->ys;
	int mcls = o->sp->header[0x16];
	if ((mcls == 3 || mcls == 0x80) && wk->x1 >= ox0 && wk->x1 - PL_XS <= tx && wk->y1 >= oy0 && wk->y1 - PL_YS <= ty) {
		if (!*hit) {                                   /* one hit per creature and step */
			*hit = 1;
			{                                                  /* 1000:172EC: a shield (ds:6AB bit = the creature's damage type A[5], power ds:6AC) swallows the hit when -damage - power is still negative as a byte */
				const uint8_t *ca = rv->vm.tail[o->thing];
				int type = ca[5], dmg = ca[1];
				if (rv->vm.g[0x11B] && !(type & 0x80) && type < 8 && (rv->vm.g[0x11B] & (1 << type))) {   /* 172EC: only the low byte of -damage is reduced by the power: a negative byte = blocked, else the hit costs 256 - byte (damage + power when that is below 256; verified with 600 cases) */
					uint8_t r = (uint8_t)(-dmg - rv->vm.g[0x11C]);
					dmg = (int8_t)r < 0 ? 0 : 256 - r;
				}
				if (dmg) acl_energy_change(&rv->vm, -dmg);
			}
			rv->vm.queue_n = 0;
			acl_thing_event(&rv->vm, &rv->mp2, o->thing, 9, 0);
			for (int q = 0; q < rv->vm.queue_n; q++)
				acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
			rv->pending_rebuild |= rv_changed(rv);
			roomview_flush_messages(rv);
		}
		return;
	}
	ColResult cr;
	collision_test(&wk->world, tx, ty, o->z0, o->xs, o->ys, o->zs, o->thing, 0, &cr);
	if (cr.zf) {
		o->x1 = tx; o->y1 = ty;
		rv->vm.pos[3 * o->thing] = (uint8_t)tx; rv->vm.pos[3 * o->thing + 1] = (uint8_t)ty;
		rv->pos_seen[3 * o->thing] = rv->vm.pos[3 * o->thing]; rv->pos_seen[3 * o->thing + 1] = rv->vm.pos[3 * o->thing + 1];
		roomobj_moved(o, gx, gy);
		*moved = 1;
	} else if ((mcls == 0x81 || mcls == 0x82 || mcls == 0x84) && cr.count != 0xFF && cr.count != 0xFE) {
		for (int k = 0; k < cr.n; k++) { shot_hits(rv, o->thing, cr.ids[k]); *hit = 1; }
	}
}

/* Creatures: things whose sprite header byte 0x16 is 3 and whose tail[5] bit 7 is clear are "active" (1000:6041, at most 6). Each game step the movement routine
 * 1000:6F62 looks at the movement state B (rec[rec[13]..]): mode 3 = velocity driven: the TAD behaviour routine of tail[1] (index + 9; verified code, tad_run_behaviour) sets
 * the sign of the velocity counters, the speed gate B[9] (low nibble counts down, at 0 it reloads from the high nibble and the creature moves) lets it step by B[6], B[7] units
 * per axis; mode 0/2 = scripted movement tables (see formats.md 6.7). */
static void creatures_step(Walker *wk, RoomView *rv) {
	if (!rv->tad.loaded)
		return;
	int active = 0, moved = 0, gw, gh, gx, gy;
	if (!roomview_geom(rv, &gw, &gh, &gx, &gy))
		return;
	int slow_skip = acl_timer_active(&rv->vm, 0x24) ? (rv->slow_phase ^= 1) & 1 : 0;
	for (int i = 0; i < rv->ro.n; i++) {
		RoomObj *o = &rv->ro.o[i];
		int h16 = o->sp->header[0x16], shot = (h16 & 0x80) != 0 && rv->vm.mstate[o->thing][0] == 3;       /* header bit 7 + velocity mode: a shot (thing 776 of level 3) */
		if ((h16 != 3 && !shot) || (rv->vm.tail[o->thing][5] & 0x80) || (rv->vm.flags[o->thing] & 0x80))
			continue;
		if (h16 == 3 && active >= 6) continue;                 /* at most 6 active creatures; shots are not counted */
		if (h16 == 3) active++;
		if (h16 == 3 && acl_timer_active(&rv->vm, 0x22)) continue;
		if (h16 == 3 && slow_skip) continue;   /* slow creature (ds:6CA): every second creature step is skipped (1000:15F56) */        /* freeze (effect 22h, ds:64D): creatures stand still (1000:171DD) */
		uint8_t *ms = rv->vm.mstate[o->thing];
		int hit = 0;
		int go = 1;
		if (ms[9]) {                                             /* speed gate */
			if ((ms[9] & 0xF) > 0) { ms[9]--; go = 0; }
			else ms[9] |= (uint8_t)(ms[9] >> 4);
		}
		if (ms[0] == 3) {
			int vx, vy;
			rv->tad.beh_thing = o->thing;
			if (g_dbg_creat && (atoi(getenv("CAD_CREAT")) < 0 || o->thing == atoi(getenv("CAD_CREAT"))) && rv->tick_dbg++ % 10 == 0) fprintf(stderr, "CREAT %d go %d beh %d at %d,%d player %d,%d\n", o->thing, go, rv->vm.tail[o->thing][1], o->x1, o->y1, wk->x1, wk->y1);
			int have = go && tad_run_behaviour(&rv->tad, &rv->vm, rv->vm.tail[o->thing][1], o->x1, o->y1, wk->x1, wk->y1, &vx, &vy) == 0;
			if (go && !have && shot) { vx = (int8_t)ms[3]; vy = (int8_t)ms[4]; have = 1; }      /* no behaviour routine: the velocity the shooter set */
			if (have) {
				int mv0 = moved;
				int dx = vx > 0 ? ms[6] : vx < 0 ? -ms[6] : 0, dy = vy > 0 ? ms[7] : vy < 0 ? -ms[7] : 0;
				if (dx) creature_move(wk, rv, o, dx, 0, gx, gy, &moved, &hit);
				if (dy) creature_move(wk, rv, o, 0, dy, gx, gy, &moved, &hit);
				if (shot && (vx || vy) && (hit || moved == mv0)) rv->vm.flags[o->thing] |= 0x80;   /* a shot that hits something or is blocked is gone */
			}
			rv->tad.beh_thing = -1;
		} else if (ms[0] == 0 || ms[0] == 2) {                   /* scripted: 00 = move by counters, 01 = jump, 02 = wait */
			if (ms[0] == 2) {
				if (ms[2] > 0 && --ms[2] == 0) ms[0] = 0;
			} else if (go) {
				const uint8_t *tab = rv->vm.mtab[o->thing];
				int tl = rv->vm.mtab_len[o->thing];
				for (int guard = 0; guard < 6; guard++) {
					int8_t *cn = (int8_t *)&ms[3];
					if (cn[0] || cn[1]) {
						int dx = cn[0] > 0 ? ms[6] : cn[0] < 0 ? -ms[6] : 0, dy = cn[1] > 0 ? ms[7] : cn[1] < 0 ? -ms[7] : 0;
						if (cn[0]) cn[0] += cn[0] > 0 ? -1 : 1;
						if (cn[1]) cn[1] += cn[1] > 0 ? -1 : 1;
						if (dx) creature_move(wk, rv, o, dx, 0, gx, gy, &moved, &hit);
						if (dy) creature_move(wk, rv, o, 0, dy, gx, gy, &moved, &hit);
						break;
					}
					int ix = ms[1];
					if (ix + 2 > tl) break;
					int op = tab[ix];
					if (op == 0 && ix + 4 <= tl) { ms[3] = tab[ix + 1]; ms[4] = tab[ix + 2]; ms[5] = tab[ix + 3]; ms[1] = (uint8_t)(ix + 4); }
					else if (op == 1) ms[1] = (uint8_t)(ix + (int8_t)tab[ix + 1]);
					else if (op == 2) { ms[2] = tab[ix + 1]; ms[0] = 2; ms[1] = (uint8_t)(ix + 2); break; }
					else if (op == 3) { ms[0] = 4; break; }                       /* stop */
					else if (op == 4) {                                           /* event 12 with the argument byte for this thing */
						ms[1] = (uint8_t)(ix + 2);
						rv->vm.queue_n = 0;
						acl_thing_event(&rv->vm, &rv->mp2, o->thing, 12, tab[ix + 1]);
						for (int q = 0; q < rv->vm.queue_n; q++)
							acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
						rv->pending_rebuild |= rv_changed(rv);
						roomview_flush_messages(rv);
					}
					else break;
				}
			}
		}
	}
	if (moved)
		walker_sync_boxes(wk, rv);
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
	wk->grab = 0;
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
		if (g_snd_log && rv->touch_age) { fprintf(stderr, "TOUCH %d verbs:", o->thing); for (int q = 0; q < rv->nverbs; q++) fprintf(stderr, " %d", rv->verbs[q]); fprintf(stderr, "\n"); }
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

/* ---- the rucksack (docs/formats.md 6.8): things whose room byte is FFh are carried; Space / Return shows the last handled one ---- */
static int ruck_list(const RoomView *rv, int *out, int max) {
	int n = 0;
	for (int t = 0; t < ACL_THINGS && n < max; t++)
		if (rv->vm.present[t] && rv->vm.room[t] == 0xFF && !(rv->vm.flags[t] & 0x80)) out[n++] = t;
	return n;
}
static const ObjSprite *thing_sprite(const RoomView *rv, int t) {
	int len;
	const uint8_t *r = mp2_thing(&rv->mp2, t, &len);
	return r && len >= 8 ? objsprites_find(&rv->objs, (r[6] << 8) | r[7]) : NULL;
}
/* verbs of a carried thing (slots as seen in the original: look, the class verb, drop, joystick) */
static void ruck_show(RoomView *rv, int t) {
	const ObjSprite *sp = thing_sprite(rv, t);
	rv->nverbs = 0;
	rv->touch_thing = t;
	rv->last_item = t;
	if (!sp) return;
	int tg = rv->ruck_target, tc = -1, ta4 = 0;
	if (tg > 0 && tg != t && tg < ACL_THINGS) {
		const ObjSprite *ts = thing_sprite(rv, tg);
		tc = ts ? ts->header[0x16] : -1;
		ta4 = rv->vm.tail[tg][4];
	}
	rv->nverbs = carried_verbs_list(sp->header[0x16], sp->header[0x17], sp->header[0x1D], (rv->vm.flags2[t] & 0x40) != 0, rv->vm.tail[t][7], tc, ta4, rv->verbs);   /* 1000:12C72, verified */
	int nm = (sp->header[10] << 8) | sp->header[11];
	rv->label_id = nm < 0x8000 ? nm : -1;
	rv->menu_sel = 0;
	if (getenv("CAD_SNDLOG")) { fprintf(stderr, "RUCK %d verbs", t); for (int i = 0; i < rv->nverbs; i++) fprintf(stderr, " %d", rv->verbs[i]); fprintf(stderr, "\n"); }
}
static void ruck_open(RoomView *rv) {
	int l[64], n = ruck_list(rv, l, 64);
	if (!n) return;
	int t = l[n - 1];
	for (int i = 0; i < n; i++) if (l[i] == rv->last_item) t = l[i];
	rv->ruck_target = (rv->touch_age < 12 && rv->nverbs && rv->touch_thing >= 0) ? rv->touch_thing : -1;
	rv->ruck_open = 1;
	rv->ruck_delay = 0;
	rv->menu_open = 1; rv->menu_released = 0;
	ruck_show(rv, t);
}
/* while the strip is shown the verb slots show the list of what was touched before the rucksack was opened (emulator), else nothing */
static void grid_verbs(RoomView *rv) {
	rv->nverbs = 0;
	const ObjSprite *ts = rv->ruck_target >= 0 ? thing_sprite(rv, rv->ruck_target) : NULL;
	if (ts) rv->nverbs = verbs_list(ts->header, rv->vm.flags2[rv->ruck_target], 0, rv->verbs);
}
static void ruck_close(RoomView *rv) {
	rv->grid_open = 0;
	rv->ruck_open = rv->menu_open = 0;
	rv->nverbs = 0;
	rv->label_id = -1;
}
/* the picture window of the right panel: black outline, brown 32 x 32 interior, the item's first frame near the middle (placement of wide sprites unverified) */
static void ruck_picture(uint8_t *px, const ObjSprite *sp) {
	for (int y = 142; y <= 175; y++)
		for (int x = 266; x <= 299; x++)
			px[y * SCREEN_W + x] = (x == 266 || x == 299 || y == 142 || y == 175) ? 0 : 14;
	if (!sp || sp->nframes < 1) return;
	const ObjFrame *f = &sp->frames[0];
	int ox = 267 + 5 + (32 - f->w) / 2, oy = 143 + (32 - f->h) / 2;
	for (int j = 0; j < f->h; j++)
		for (int i = 0; i < f->w; i++) {
			int x = ox + i, y = oy + j;
			if (f->pixels[j * f->w + i] && x >= 267 && x <= 298 && y >= 143 && y <= 174) px[y * SCREEN_W + x] = f->pixels[j * f->w + i];
		}
}

/* the picture scroll shown at the top left while a carried thing is looked at: a text-less box at (16, 10) (5 fill columns, 4 rows) with a green double frame around the
 * thing's picture (outer ring colour 1 at (23,25)-(57,59), inner ring 3, brown interior; sprite origin = window (24, 26) + (5, 10), as in the panel window) */
static void grid_draw(uint8_t *px, const RoomView *rv) {
	int l[64], n = ruck_list(rv, l, 64);
	if (n < 1 || !g_tiles) return;
	int R = (n - 1) / 4, vis = (R < 3 ? R : 3) + 1;                      /* rows shown (at most 4) */
	int cols = n < 4 ? n : 4;
	message_box(px, 16, 10, 5 * cols, 5 * vis - 1, g_tiles, g_tiles_len, msg_rnd, NULL);
	for (int r = 0; r < vis; r++)
		for (int c = 0; c < 4; c++) {
			int k = (rv->gscroll + r) * 4 + c;
			if (k >= n) break;
			int cx = 23 + 40 * c, cy = 25 + 40 * r, sel = (c == rv->gx && r == rv->gvis);
			for (int y = cy; y <= cy + 34; y++)
				for (int x = cx; x <= cx + 34; x++) {
					int edge = (x == cx || x == cx + 34 || y == cy || y == cy + 34), edge2 = (x == cx + 1 || x == cx + 33 || y == cy + 1 || y == cy + 33);
					px[y * SCREEN_W + x] = (uint8_t)(sel ? (edge ? 1 : edge2 ? 3 : 14) : (edge ? 4 : 14));
				}
			const ObjSprite *sp = thing_sprite(rv, l[k]);
			if (!sp || sp->nframes < 1) continue;
			const ObjFrame *f = &sp->frames[0];
			int ox = cx + 1 + 5 + (32 - f->w) / 2, oy = cy + 1 + (32 - f->h) / 2;
			for (int j = 0; j < f->h; j++)
				for (int i = 0; i < f->w; i++) {
					int x = ox + i, y = oy + j;
					if (f->pixels[j * f->w + i] && x >= cx + 2 && x <= cx + 32 && y >= cy + 2 && y <= cy + 32) px[y * SCREEN_W + x] = f->pixels[j * f->w + i];
				}
		}
}
static void look_picture(uint8_t *px, const ObjSprite *sp) {
	message_box(px, 16, 10, 5, 4, g_tiles, g_tiles_len, msg_rnd, NULL);
	for (int y = 25; y <= 59; y++)
		for (int x = 23; x <= 57; x++) {
			int ring = (x == 23 || x == 57 || y == 25 || y == 59) ? 1 : (x == 24 || x == 56 || y == 26 || y == 58) ? 3 : 14;
			px[y * SCREEN_W + x] = (uint8_t)ring;
		}
	if (!sp || sp->nframes < 1) return;
	const ObjFrame *f = &sp->frames[0];
	int ox = 24 + 5 + (32 - f->w) / 2, oy = 26 + (32 - f->h) / 2;
	for (int j = 0; j < f->h; j++)
		for (int i = 0; i < f->w; i++) {
			int x = ox + i, y = oy + j;
			if (f->pixels[j * f->w + i] && x >= 25 && x <= 55 && y >= 27 && y <= 57) px[y * SCREEN_W + x] = f->pixels[j * f->w + i];
		}
}

/* ---- the map (F1, 1000:369F / 3806 / 38C3): a scroll at (96, 30) (14 fill columns, 9 rows); every room entered is an outline of its record's size (bytes 4, 5) times the zoom
 * at ((byte 1 - ox) * zoom + 6Bh, (byte 3 - oy) * zoom + 29h), colour 4, the current room colour 10, clipped to the window (6Bh..D5h, 29h..73h); the centre is the current room ---- */
static void map_centre(RoomView *rv) {
	const uint8_t *rec = rv->room >= 0 && rv->room < rv->mp2.sec[0].count ? rv->mp2.sec[0].rec[rv->room] : NULL;
	if (!rec || rv->mp2.sec[0].len[rv->room] < 6) return;
	rv->map_ox = rec[1] - 0x35 / rv->map_zoom + rec[4] / 2;
	rv->map_oy = rec[3] - 0x25 / rv->map_zoom + rec[5] / 2;
}
static void map_open_view(RoomView *rv) {
	if (!rv->ok) return;
	if (rv->map_zoom < 1) rv->map_zoom = 2;
	map_centre(rv);
	rv->map_open = 1;
}
static void map_open_magic(RoomView *rv) { rv->map_magic = 1; map_open_view(rv); }
static void map_draw(uint8_t *px, const RoomView *rv) {
	if (!g_tiles || !g_font3) return;
	message_box(px, 96, 30, 14, 9, g_tiles, g_tiles_len, msg_rnd, NULL);
	int vis = 0;
	for (int i = 0; i < 128; i++) vis += rv->visited[i];
	char tx[64];
	snprintf(tx, sizeof(tx), "%s%d", rv->names.count > 42 ? rv->names.strings[42] : "ROOMS ENTERED ", vis);
	message_text(px, 0x6B, 0x74, tx, g_font3, g_font3_len);
	for (int r = 0; r < rv->mp2.sec[0].count && r < 128; r++) {
		const uint8_t *rec = rv->mp2.sec[0].rec[r];
		if (!rec || rv->mp2.sec[0].len[r] < 6) continue;
		if (rv->map_magic ? (rv->mp2.sec[0].len[r] > 0x17 && (rec[0x17] & 0x80)) : !rv->visited[r]) continue;
		int z = rv->map_zoom, w = rec[4] * z, h = rec[5] * z;
		int x = (rec[1] - rv->map_ox) * z + 0x6B, y = (rec[3] - rv->map_oy) * z + 0x29;
		int col = r == rv->room ? 10 : 4;
		for (int i = 0; i <= w; i++)
			for (int j = 0; j <= h; j++) {
				if (i && i != w && j && j != h) continue;
				int xx = x + i, yy = y + j;
				if (xx > 0x6B && xx <= 0xD5 && yy > 0x29 && yy <= 0x73) px[yy * SCREEN_W + xx] = (uint8_t)col;
			}
	}
}
/* one frame of the map: arrows scroll, Return zooms in, '-' out, F2 re-centres, End leaves */
static void map_step(RoomView *rv, int keys) {
	int ret = (keys & 0x20) != 0, rte = ret && !rv->ret_prev;
	rv->ret_prev = ret;
	int minus = (keys & 0x40) != 0, me = minus && !rv->map_prev;
	rv->map_prev = minus;
	if (keys & 0x80) { rv->map_open = 0; rv->map_magic = 0; return; }
	if (keys & 4) rv->map_ox--;
	if (keys & 8) rv->map_ox++;
	if (keys & 1) rv->map_oy--;
	if (keys & 2) rv->map_oy++;
	if (rte && rv->map_zoom < 10) { rv->map_zoom++; rv->map_ox += 0x1A / rv->map_zoom; rv->map_oy += 0x12 / rv->map_zoom; }
	if (me && rv->map_zoom > 1) { rv->map_zoom--; rv->map_ox -= 0x1A / rv->map_zoom; rv->map_oy -= 0x12 / rv->map_zoom; }
}

/* run the chosen verb (handler table 1000:2E0B): 11 = look (event 16 when the thing has flag 0x20, else message 3), 4 and 7 = event 5, 2 = event 0;
 * the other verbs (take, drop, use ...) are not decoded yet */
static void menu_choose(RoomView *rv, int verb) {
	int t = rv->touch_thing;
	if (g_snd_log) fprintf(stderr, "VERB %d thing %d\n", verb, t);
	rv->vm.queue_n = 0;
	if (verb == 11) {
		if (t >= 0 && t < ACL_THINGS && (rv->vm.flags[t] & 0x20)) acl_thing_event(&rv->vm, &rv->mp2, t, 16, 0);
		else {
			const RoomObj *lo = NULL;
			for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == t) { lo = &rv->ro.o[i]; break; }
			const ObjSprite *lsp = lo ? lo->sp : thing_sprite(rv, t);
			int hc = lsp ? lsp->header[0x16] : -1;
			if (hc == 8 && t >= 0 && t < ACL_THINGS) {                      /* 1000:8615: "THIS IS OPEN" / "THIS IS CLOSED AND LOCKED." + the trap notice */
				const uint8_t *ta = rv->vm.tail[t];
				char tx[200];
				if (ta[4] & 1) snprintf(tx, sizeof(tx), "%s", lng_str(rv, 22));
				else snprintf(tx, sizeof(tx), "%s%s", lng_str(rv, 23), lng_str(rv, (ta[2] || ta[3]) ? 24 : 25));
				if (ta[5]) { size_t l = strlen(tx); snprintf(tx + l, sizeof(tx) - l, " %s", lng_str(rv, 0x33)); }
				msg_show_text(rv, tx);
			} else if (hc >= 0 && (hc == 1 || hc == 2 || hc == 4 || (hc & 0x80))) {   /* spell / potion / weapon descriptions (1000:84D9) */
				char tx[200];
				if (thing_description(rv, t, lsp, tx, sizeof(tx))) msg_show_text(rv, tx);
			} else if (rv->ruck_open && lsp) {                              /* a carried thing: "THIS IS A <name>" (emulator: the diary) */
				int nm = (lsp->header[10] << 8) | lsp->header[11];
				char tx[200];
				snprintf(tx, sizeof(tx), "%s%s", lng_str(rv, 5), nm < 0x8000 ? lng_str(rv, nm) : "");
				msg_show_text(rv, tx);
			} else if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 3;
		}
	} else if (verb == 4 || verb == 7) {      /* 1000:3159 / 3163: sound 30h / 3Dh, then event 5 */
		snd_play(verb == 4 ? 0x30 : 0x3D);
		acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
	} else if (verb == 8) {           /* 1000:2FD4: on a thing with header byte 16h = 7 (the diary) the status dialog, otherwise event 5 (3183) */
		const RoomObj *o = NULL;
		for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == t) { o = &rv->ro.o[i]; break; }
		if (o && o->sp->header[0x16] == 7) rv->show_status = 1;
		else acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
	} else if (verb == 13) {          /* 1000:340D: hold / unhold; holding queues event 27 (1Bh) */
		if (rv->held == t) rv->held = 0;
		else { rv->held = t; acl_thing_event(&rv->vm, &rv->mp2, t, 27, 0); }
	} else if (verb == 10) {          /* 1000:336A: grab - ds:74 = the touched thing; walking straight away from it drags it along (walker_step, 1000:10732) */
		if (t >= 0 && t < ACL_THINGS) g_walk.grab = t + 1;
	} else if (verb == 16) {          /* 1000:2E40: event 1 */
		acl_thing_event(&rv->vm, &rv->mp2, t, 1, 0);
	} else if (verb == 2) {
		int l[64];
		if (ruck_list(rv, l, 64) >= rv->vm.cap) {                /* 1000:2EA9: the capacity ds:5A0 (20h, 28h after level 4's op 3Eh) is reached -> message 35h */
			if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x35;
		} else if (t >= 0 && t < ACL_THINGS) {
			acl_thing_event(&rv->vm, &rv->mp2, t, 0, 0);          /* event 0 first (the handlers of coins delete the thing) */
			if (!(rv->vm.flags[t] & 0x80)) { rv->vm.room[t] = 0xFF; rv->vm.flags[t] &= (uint8_t)~0x80; rv->last_item = t; }   /* carried */
			rv->pending_rebuild = 1;
			rv->nverbs = 0; rv->label_id = -1;
		}
	} else if (verb == 3) {           /* 1000:31A3: open / close a chest; the content is placed on top of it (the original's collision-aware placement is approximated by raising z) */
		int content = 0;
		if (acl_chest_toggle(&rv->vm, &rv->mp2, t, &content)) {
			snd_play(0x38);
			if (content > 0 && content < ACL_THINGS) {
				int len, cz = rv->vm.pos[3 * t + 2];
				const uint8_t *cr = mp2_thing(&rv->mp2, content, &len);
				const ObjSprite *sp = cr && len >= 8 ? objsprites_find(&rv->objs, (cr[6] << 8) | cr[7]) : NULL;
				int xs = sp ? (sp->header[16] << 8 | sp->header[17]) : 7, ys = sp ? (sp->header[18] << 8 | sp->header[19]) : 7, zs = sp ? (sp->header[20] << 8 | sp->header[21]) : 7;
				int x = rv->vm.pos[3 * t], y = rv->vm.pos[3 * t + 1], z = cz;
				for (; z < 0x2C; z++) {
					ColResult cr2;
					collision_test(&g_walk.world, x, y, z, xs, ys, zs, content, 0, &cr2);
					if (cr2.zf) break;
				}
				rv->vm.pos[3 * content] = (uint8_t)x; rv->vm.pos[3 * content + 1] = (uint8_t)y; rv->vm.pos[3 * content + 2] = (uint8_t)z;
				rv->vm.room[content] = (uint8_t)rv->room;
				rv->vm.flags[content] &= 0x7F;           /* inside a container -> in the room */
			}
			acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
		}
	} else if (verb == 14) {          /* 1000:3483: class 7 things get event 5, everything else is removed (queued for deletion, 1000:62A9) */
		const RoomObj *o = NULL;
		for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == t) { o = &rv->ro.o[i]; break; }
		if (o && o->sp->header[0x17] == 7) acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
		else if (t >= 0 && t < ACL_THINGS) { rv->vm.flags[t] |= 0x80; snd_play(0x0A); }
	}
	for (int q = 0; q < rv->vm.queue_n; q++)
		acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	if (rv->vm.describe) { rv->look_pic = rv->vm.describe; rv->vm.describe = 0; }
	rv->pending_rebuild |= rv_changed(rv);
	if (g_snd_log) fprintf(stderr, "  after verb: money %u flags %02x room %d msg_n %d\n", rv->vm.money, t >= 0 ? rv->vm.flags[t] : 0, t >= 0 ? rv->vm.room[t] : -1, rv->vm.msg_n);
	roomview_flush_messages(rv);
}

/* ---- thrown things (docs/formats.md, "Throwing"): the z path FBA5 (rise, then fall; after its end the fall path FB8E, speed up to -8 per step) and the horizontal path by direction (steps of
 * 4 4 3 3 3 2 2 2 2 2 1 1 1 1 1 1 1 1 along the facing axes), one entry per game step; a blocked horizontal step ends the horizontal path, a blocked fall lands the thing ---- */
static const int8_t zthrow[17] = {6, 6, 6, 4, 4, 2, 1, 0, 0, -1, -2, -4, -4, -6, -6, -6, -7};
static const int8_t zfall[20] = {-1, -1, -2, -2, -3, -3, -3, -4, -4, -5, -5, -5, -6, -6, -6, -7, -7, -7, -7, -8};
static const int8_t hmag[18] = {4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1};
static void thing_box(const RoomView *rv, int t, int *xs, int *ys, int *zs) {
	const ObjSprite *sp = thing_sprite(rv, t);
	*xs = sp ? (sp->header[16] << 8 | sp->header[17]) : 7;
	*ys = sp ? (sp->header[18] << 8 | sp->header[19]) : 7;
	*zs = sp ? (sp->header[20] << 8 | sp->header[21]) : 7;
}
/* the spot in front of the player for thing t (1000:6C72); returns 0 when it is blocked */
static int front_spot_h(Walker *wk, RoomView *rv, int t, int *ox, int *oy, int *oz, int zadd);
static int front_spot(Walker *wk, RoomView *rv, int t, int *ox, int *oy, int *oz) { return front_spot_h(wk, rv, t, ox, oy, oz, 0); }
static int front_spot_h(Walker *wk, RoomView *rv, int t, int *ox, int *oy, int *oz, int zadd) {
	int xs, ys, zs;
	thing_box(rv, t, &xs, &ys, &zs);
	int dxp = wk->x1, dyp = wk->y1;
	int face = wk->face;
	if (face != 1 && face != 2 && face != 4 && face != 8 && face != 5 && face != 6 && face != 9 && face != 10)   /* no move seen yet: the facing */
		face = wk->dir == DIR_RIGHT ? 8 : wk->dir == DIR_LEFT ? 4 : wk->dir == DIR_DOWN ? 2 : 1;
	place_spot(face, wk->x1, wk->y1, wk->x1 - (PL_XS - 1), wk->y1 - (PL_YS - 1), xs, ys, &dxp, &dyp);   /* 1000:6C72, the eight cases */
	ColResult cr;
	collision_test(&wk->world, dxp, dyp, wk->z + zadd, xs, ys, zs, t, 0, &cr);
	*ox = dxp; *oy = dyp; *oz = wk->z + zadd;
	if (g_snd_log) fprintf(stderr, "SPOT thing %d at %d,%d z %d size %d,%d,%d: zf %d count %02x flag %02x n %d ids %d\n", t, dxp, dyp, wk->z + zadd, xs, ys, zs, cr.zf, cr.count, cr.flag, cr.n, cr.n ? cr.ids[0] : -1);
	return cr.zf;
}
/* spell 0 / weapon strike (TAD routine 085C, level 1): the first creature in the line of fire loses the item's power A[1] from its hit points A[0]; at underflow event 23 (its death) */
static void cast_strike(Walker *wk, RoomView *rv, int item) {
	uint8_t *a = rv->vm.tail[item];
	int best = -1, bd = 1 << 30;
	int fx = wk->dir == DIR_RIGHT ? 1 : wk->dir == DIR_LEFT ? -1 : 0, fy = wk->dir == DIR_DOWN ? 1 : wk->dir == DIR_UP ? -1 : 0;
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		if (o->sp->header[0x16] != 3 || (rv->vm.flags[o->thing] & 0x80) || (rv->vm.tail[o->thing][5] & 0x80)) continue;
		int ox = rv->vm.pos[3 * o->thing] - wk->x1, oy = rv->vm.pos[3 * o->thing + 1] - wk->y1;
		int along = ox * fx + oy * fy, across = fx ? oy : ox;
		if (along <= 0 || abs(across) > 7 || along >= bd) continue;
		best = o->thing; bd = along;
	}
	if (best < 0) return;
	uint8_t *hp = &rv->vm.tail[best][0];
	if (*hp < a[1]) {                                              /* underflow: the byte wraps and the creature dies (event 23) */
		*hp = (uint8_t)(*hp - a[1]);
		rv->vm.queue_n = 0;
		acl_thing_event(&rv->vm, &rv->mp2, best, 23, 0);
		for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
		rv->pending_rebuild |= rv_changed(rv);
	} else *hp = (uint8_t)(*hp - a[1]);
	roomview_flush_messages(rv);
}
/* spell 1 (TAD routine 08C5): up to A[1] active creatures that are not immune (A[6] bit 0 clear) die (event 23) */
static void cast_smite(Walker *wk, RoomView *rv, int item) {
	(void)wk;
	uint8_t *a = rv->vm.tail[item];
	int left = a[1];
	for (int i = 0, active = 0; i < rv->ro.n && active < 6 && left > 0; i++) {
		const RoomObj *o = &rv->ro.o[i];
		int t = o->thing;
		if (o->sp->header[0x16] != 3 || (rv->vm.flags[t] & 0x80) || (rv->vm.tail[t][5] & 0x80)) continue;
		active++;
		left--;
		if (rv->vm.tail[t][6] & 1) continue;
		rv->vm.queue_n = 0;
		acl_thing_event(&rv->vm, &rv->mp2, t, 23, 0);
		for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	}
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
}
static void menu_choose_thing(RoomView *rv, int t, int verb) { int save = rv->touch_thing; rv->touch_thing = t; menu_choose(rv, verb); rv->touch_thing = save; }

/* the nearest room thing in the facing line (within the player's width, up to `reach` units) that satisfies the sprite class test (-1: any) */
static int line_target(Walker *wk, RoomView *rv, int h16, int reach) {
	int best = -1, bd = 1 << 30;
	int fx = wk->dir == DIR_RIGHT ? 1 : wk->dir == DIR_LEFT ? -1 : 0, fy = wk->dir == DIR_DOWN ? 1 : wk->dir == DIR_UP ? -1 : 0;
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		int t = o->thing;
		if (t == 0 || (rv->vm.flags[t] & 0x80) || (h16 >= 0 && o->sp->header[0x16] != h16)) continue;
		int ox = rv->vm.pos[3 * t] - wk->x1, oy = rv->vm.pos[3 * t + 1] - wk->y1;
		int along = ox * fx + oy * fy, across = fx ? oy : ox;
		if (along <= -(fx ? o->xs : o->ys) || along > reach || abs(across) > 8 + (fx ? o->ys : o->xs) / 2) continue;
		if (along < 0) along = 0;
		if (along < bd) { bd = along; best = t; }
	}
	return best;
}
/* spells 7 / 8 (TAD routines 09F5 / 0A48): 7 opens a closed, unlocked-or-lockable CHEST in front (the lock word A[2..3] := 0, then verb 3), 8 clears the lock of a thing (A[2..3] := 0, A[7] bit 6 cleared) */
static void cast_unlock(Walker *wk, RoomView *rv, int item, int spell) {
	(void)item;
	int t = line_target(wk, rv, spell == 7 ? 8 : -1, 24);
	uint8_t *ta = t > 0 ? rv->vm.tail[t] : NULL;
	int bad = !ta || (spell == 7 ? ((ta[4] & 1) || (ta[4] & 0x40)) : (ta[7] & 0x20));
	if (bad) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x2F; roomview_flush_messages(rv); return; }
	ta[2] = ta[3] = 0;
	if (spell == 8) { ta[7] &= (uint8_t)~0x40; if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 1; roomview_flush_messages(rv); return; }
	ta[4] &= (uint8_t)~2;
	menu_choose_thing(rv, t, 3);                                       /* verb 3 on the chest */
}
/* spell 5 (TAD routine 097A, mind blast): every creature on the room's creature list (at most 6 active ones, as for spell 1) that is not immune
   (A[6] bit 0 clear) loses the item's power A[1] from its hit points A[0]; at underflow event 23 (its death) */
static void cast_blast(Walker *wk, RoomView *rv, int item) {
	(void)wk;
	int power = rv->vm.tail[item][1];
	int any = 0;
	for (int i = 0; i < rv->ro.n; i++) { const RoomObj *o = &rv->ro.o[i]; if (o->sp->header[0x16] == 3 && !(rv->vm.flags[o->thing] & 0x80) && !(rv->vm.tail[o->thing][5] & 0x80)) any = 1; }
	if (!any) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x2F; roomview_flush_messages(rv); return; }   /* 097A: an empty creature list = "YOU WASTED THAT SPELL" (emulator) */
	for (int i = 0, active = 0; i < rv->ro.n && active < 6; i++) {
		const RoomObj *o = &rv->ro.o[i];
		int t = o->thing;
		if (o->sp->header[0x16] != 3 || (rv->vm.flags[t] & 0x80) || (rv->vm.tail[t][5] & 0x80)) continue;
		active++;
		if (rv->vm.tail[t][6] & 1) continue;
		uint8_t *hp = &rv->vm.tail[t][0];
		if (*hp >= power) { *hp = (uint8_t)(*hp - power); continue; }
		*hp = (uint8_t)(*hp - power);
		rv->vm.queue_n = 0;
		acl_thing_event(&rv->vm, &rv->mp2, t, 23, 0);
		for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	}
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
}
/* the player's projectile leaves in the facing direction (1000:16A37: the thing is placed at the shooter, B[3], B[4] = +-127 from the facing) */
static void launch_shot(Walker *wk, RoomView *rv, int id) {
	int fx = (wk->face & 8) ? 1 : (wk->face & 4) ? -1 : 0, fy = (wk->face & 2) ? 1 : (wk->face & 1) ? -1 : 0;
	if (!fx && !fy) { fx = wk->dir == DIR_RIGHT ? 1 : wk->dir == DIR_LEFT ? -1 : 0; fy = wk->dir == DIR_DOWN ? 1 : wk->dir == DIR_UP ? -1 : 0; }
	acl_place_shot(&rv->vm, id, rv->room, wk->x1 + 2 * fx, wk->y1 + 2 * fy, wk->z, 127 * fx, 127 * fy);
	rv->pending_rebuild = 1;
}
/* fire with a held throwing weapon (class 4, 1000:69CF): A[0..1] = the ammunition thing flown (the weapon needs no ammunition in the rucksack when A[2] = 0), sound 14h,
   the shots A[2] of the ammunition drop (FFh = unlimited): at 0 message 15, and a weapon with A[4] bit 0 is used up too */
static void fire_weapon(Walker *wk, RoomView *rv, int item) {
	uint8_t *a = rv->vm.tail[item];
	int ammo = (a[0] << 8) | a[1];
	if (a[2] != 0 || ammo <= 0 || ammo >= ACL_THINGS || !rv->vm.present[ammo] || rv->vm.tail[ammo][2] == 0) {
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 14;
		roomview_flush_messages(rv);
		return;
	}
	snd_play(0x14);
	rv->fire_cool = a[3];                                              /* 169CF: ds:629 := A[3] */
	launch_shot(wk, rv, ammo);
	uint8_t *sh = &rv->vm.tail[ammo][2];
	if (*sh != 0xFF && --*sh == 0) {
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 15;
		if (a[4] & 1) { rv->vm.flags[item] |= 0x80; if (rv->held == item) rv->held = 0; }
		roomview_flush_messages(rv);
	}
}
/* the spell routines of the "cast on an object" mode (A[7] = 2; 1000:169AB and the TAD table entries 0A74 ...): the target is the thing just touched (message 41 when there is none);
   a spell that does not fit the target's class is "wasted" (message 47). */
static void cast_on_target(RoomView *rv, int item, int t) {
	uint8_t *a = rv->vm.tail[item];
	int sp = spell_sem(rv, a[0]), p = a[1];
	const ObjSprite *ts = thing_sprite(rv, t);
	int cls = ts ? ts->header[0x16] : -1;
	uint8_t *ta = rv->vm.tail[t];
	int msg = -1, ok = 0;
	switch (sp) {
	case 10:                                                          /* BLESS WEAPON (0A74): adds the power to the weapon's shots (class 4: those of its ammunition thing) */
		if (cls == 4) {
			int am = (ta[0] << 8) | ta[1];
			if (ta[2] == 0 && am > 0 && am < ACL_THINGS) { int v = rv->vm.tail[am][1] + p; rv->vm.tail[am][1] = (uint8_t)(v > 255 ? 255 : v); ok = 1; msg = 0x20; }
		} else if (cls & 0x80) { int v = ta[1] + p; ta[1] = (uint8_t)(v > 255 ? 255 : v); ok = 1; msg = 0x20; }
		break;
	case 13: case 14:                                                 /* BLESS MAGIC / BLESS POTION (0AF5 / 0B09): the power is added to a spell item / a potion */
		if (cls == (sp == 13 ? 1 : 2)) { int v = ta[1] + p; ta[1] = (uint8_t)(v > 255 ? 255 : v); ok = 1; }
		break;
	case 15:                                                          /* DISPEL TRAP (0B40): a trapped chest */
		if (cls == 8 && ta[5]) { ta[5] = 0; ok = 1; msg = 0x34; }
		break;
	case 19:                                                          /* PURIFY POTION (0B74) */
		if (cls == 2 && (ta[3] & 4)) { ta[3] &= (uint8_t)~4; ok = 1; msg = 0x24; }
		break;
	case 20: case 21:                                                 /* READ MAGIC / LEARN POTION (0BA3 / 0BDF): the name of the item becomes known */
		if (cls == (sp == 20 ? 1 : 2) && (ta[3] & 1)) { ta[3] &= (uint8_t)~0x83; ok = 1; }
		break;
	case 28:                                                          /* 0BFD (Payoff 27 BANISH): a creature loses the power from its hit points, event 23 at underflow */
		if (cls == 3) {
			ok = 1;
			if (ta[0] < p) { ta[0] = (uint8_t)(ta[0] - p); rv->vm.queue_n = 0; acl_thing_event(&rv->vm, &rv->mp2, t, 23, 0);
				for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
				rv->pending_rebuild |= rv_changed(rv); }
			else ta[0] = (uint8_t)(ta[0] - p);
		}
		break;
	default: return;
	}
	if (!ok) msg = 0x2F;
	if (msg >= 0 && rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = msg;
	roomview_flush_messages(rv);
}
/* spell numbers of the two games' TAD tables, reduced to Cadaver 1's numbering (the Payoff inserted TRANSLATE / TELEPORT / REVEAL ... and moved MAP to 30; its routines match by name, see formats.md) */
static int spell_sem(const RoomView *rv, int n) {
	if (!rv->use_tata) return n;
	static const int8_t pay[31] = {0, 1, 2, -1, 5, -1, 7, 8, -1, 10, 11, -1, 13, 14, 15, -1, -1, 18, 19, 20, 21, 22, 0, -1, -1, -1, -1, 28, -1, -1, 4};
	return n >= 0 && n < 31 ? pay[n] : -1;
}
/* fire with a held spell item (1000:6896): refused with message 25h while A[3] bit 7 is set; sound 11h (A[7] = 0, the projectile mode) or 06h,
   the spell's effect, then the charge count A[2] drops (16AC9: FFh = unlimited; at 0 the item is gone: message 0Fh and it leaves the hand).
   Only the spells 0, 1, 2, 4, 5, 7 and 8 have an effect in the port; the others are ignored (no charge used) */
static void cast_spell_item(Walker *wk, RoomView *rv, int item) {
	uint8_t *a = rv->vm.tail[item];
	int sp = spell_sem(rv, a[0]);
	if (a[3] & 0x80) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x25; roomview_flush_messages(rv); return; }
	snd_play(a[7] == 0 ? 0x11 : 0x06);
	rv->fire_cool = a[6];                                               /* 16AC9 / 169A1: ds:629 := A[6] */
	if (a[7] == 1) {                                                    /* 1000:168F0: mode 1 only: room event 24 (0x4018), argument = the spell number (23 room handlers: magic doors / altars); checked by running 6896 in the emulator */
		rv->vm.queue_n = 0;
		acl_room_event(&rv->vm, &rv->mp2, rv->room, 24, a[0]);
		rv->pending_rebuild |= rv_changed(rv);
		roomview_flush_messages(rv);
	}
	int proj = (a[4] << 8) | a[5];
	if (a[7] == 2) {                                                    /* the mode 'cast on an object' (169AB) */
		int tt = rv->cast_target > 0 ? rv->cast_target : -1;
		if (tt < 0 || tt >= ACL_THINGS) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x29; roomview_flush_messages(rv); return; }
		cast_on_target(rv, item, tt);
	} else if (sp == 18 || sp == 22) acl_set_timer(&rv->vm, sp == 18 ? 0x25 : 0x26, a[1]);   /* CONFUSION / TURN MONSTER (0B6E / 0BF7): effect timer 25h / 26h; the creature behaviours test ds:650 (random direction) / ds:651 (flee: the direction + 4) */
	else if (sp == 11) acl_set_timer(&rv->vm, 0x24, a[1]);            /* SLOW CREATURE (0AE9): ds:6CA = 1 and effect timer 24h */
	else if (sp == 0 && a[7] == 0 && proj > 0 && proj < ACL_THINGS && rv->vm.present[proj]) {   /* the projectile thing flies and strikes on contact (1000:6896 / 17250) */
		rv->vm.tail[proj][0] = (uint8_t)(a[0] | 0x80); rv->vm.tail[proj][1] = a[1]; rv->vm.tail[proj][5] = a[8];
		launch_shot(wk, rv, proj);
	} else if (sp == 0) cast_strike(wk, rv, item);
	else if (sp == 1) cast_smite(wk, rv, item);
	else if (sp == 4) map_open_magic(rv);
	else if (sp == 2) acl_set_timer(&rv->vm, 0x22, a[1]);               /* freeze (TAD 093E): effect timer 22h for A[1] units */
	else if (sp == 5) cast_blast(wk, rv, item);
	else if (sp == 7 || sp == 8) cast_unlock(wk, rv, item, sp);
	rv->vm.queue_n = 0;                                                 /* 1000:1691B (modes 1 and 2, not the projectile mode 0): event 25 (19h) for the spell item itself: scripted spells (level 3's thing 805, Payoff thing 279) */
	if (a[7] != 0) acl_thing_event(&rv->vm, &rv->mp2, item, 25, 0);
	for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
	if (a[2] != 0xFF && --a[2] == 0) {
		rv->vm.flags[item] |= 0x80;
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x0F;
		if (rv->held == item) rv->held = 0;
		rv->pending_rebuild |= rv_changed(rv);
		roomview_flush_messages(rv);
	}
}
/* fire with a held thing: sound 0Eh, the thing is placed in front of the player (event 21) and starts to fly */
static void throw_held(Walker *wk, RoomView *rv) {
	int t = rv->held, x, y, z;
	{
		const ObjSprite *hs = t > 0 && t < ACL_THINGS ? thing_sprite(rv, t) : NULL;
		if (hs && hs->header[0x16] == 1) { cast_spell_item(wk, rv, t); return; }
		if (hs && hs->header[0x16] == 4) { fire_weapon(wk, rv, t); return; }                                 /* a throwing weapon (1000:69CF) */                            /* a spell item (1000:6896) */
	}
	if (t <= 0 || t >= ACL_THINGS || rv->vm.room[t] != 0xFF || rv->nfly >= 8) return;
	snd_play(0x0E);
	if (!front_spot_h(wk, rv, t, &x, &y, &z, PL_ZS - 1)) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 27; roomview_flush_messages(rv); return; }      /* 16C72 with cl = E2h: the spot at the player's z + the full height of its box (zt - zb = 29) */
	rv->vm.pos[3 * t] = (uint8_t)x; rv->vm.pos[3 * t + 1] = (uint8_t)y; rv->vm.pos[3 * t + 2] = (uint8_t)z;
	rv->vm.room[t] = (uint8_t)rv->room;
	rv->loose[t] = 1;
	rv->held = 0;
	int db = wk->dir == DIR_RIGHT ? 8 : wk->dir == DIR_LEFT ? 4 : wk->dir == DIR_DOWN ? 2 : 1;
	{ const ObjSprite *ws = thing_sprite(rv, t); rv->fly[rv->nfly].weightless = ws && ws->header[13] == 0 && 0; }       /* 12F1C: weight byte 0Dh = 0 -> the vertical path FBA3 (a bare 00) instead of FBA5; never the case for an object that has had an entry, because 14A2E / 151E2 patch a zero weight to 0Ah (the sprite header is shared) */
	rv->fly[rv->nfly].thing = t; rv->fly[rv->nfly].zi = 0; rv->fly[rv->nfly].hi = 0; rv->fly[rv->nfly].dirbits = db; rv->fly[rv->nfly++].fall = 0;
	rv->vm.queue_n = 0;
	acl_thing_event(&rv->vm, &rv->mp2, t, 21, 0);
	for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	rv->pending_rebuild = 1;
	roomview_flush_messages(rv);
}
/* gravity (1000:1572B / 6BFC): a loose thing (sprite header byte 0Ch bit 0 set = non-solid, record flags2 bits 4 and 8 clear) that stands above the floor with nothing under it starts to fall */
static void gravity_scan(Walker *wk, RoomView *rv) {
	for (int i = 0; i < rv->ro.n && rv->nfly < 8; i++) {
		const RoomObj *o = &rv->ro.o[i];
		int t = o->thing, z = (int8_t)rv->vm.pos[3 * t + 2];
		if (z <= 0 || !(o->sp->header[12] & 1) || (rv->vm.flags2[t] & 0x0C) || (rv->vm.flags[t] & 0x80) || t == 0) continue;   /* entry flag 27h bit 0 = sprite header byte 0Ch bit 0 (the non-solid things: items) */
		int busy = 0;
		for (int k = 0; k < rv->nfly; k++) if (rv->fly[k].thing == t) busy = 1;
		if (busy) continue;
		int xs, ys, zs;
		thing_box(rv, t, &xs, &ys, &zs);
		ColResult cr;
		collision_test(&wk->world, rv->vm.pos[3 * t], rv->vm.pos[3 * t + 1], z - 1, xs, ys, zs, t, 0, &cr);
		if (!cr.zf) continue;
		rv->fly[rv->nfly].thing = t; rv->fly[rv->nfly].zi = 0; rv->fly[rv->nfly].hi = 18; rv->fly[rv->nfly].dirbits = 0; rv->fly[rv->nfly].weightless = 0; rv->fly[rv->nfly++].fall = 1;
	}
}
static void fly_step(Walker *wk, RoomView *rv) {
	for (int k = 0; k < rv->nfly; k++) {
		int t = rv->fly[k].thing, xs, ys, zs;
		thing_box(rv, t, &xs, &ys, &zs);
		int x = rv->vm.pos[3 * t], y = rv->vm.pos[3 * t + 1], z = (int8_t)rv->vm.pos[3 * t + 2];
		/* 1000:16D3A: the vertical path (FBA5: 06 06 06 04 04 02 01 00 00 FF FE FC FC FA FA FA F9, FBA3 for a weightless thing: 00; FB8E for a fall) holds its last value
		   while the byte after it is 50h; the horizontal path (4 4 3 3 3 2 2 2 2 2 1 ...) ends in 00 00 (no more horizontal move). The combined step is tested with 116E0: when it is free
		   it is applied (16E85); when it is blocked the horizontal path is dropped and the vertical path restarts as the fall FB8E, the same step is tried again with that (16E5D); a
		   second blocked step while falling ends the flight (16E74 -> 16F53); a step that brings z to exactly 0 downwards is the landing on the floor (16E91) */
		int dz = rv->fly[k].fall ? zfall[rv->fly[k].zi < 20 ? rv->fly[k].zi : 19] : rv->fly[k].weightless ? 0 : zthrow[rv->fly[k].zi < 17 ? rv->fly[k].zi : 16];
		int m = rv->fly[k].hi < 18 ? hmag[rv->fly[k].hi] : 0;
		int dx = (rv->fly[k].dirbits & 8 ? m : rv->fly[k].dirbits & 4 ? -m : 0), dy = (rv->fly[k].dirbits & 2 ? m : rv->fly[k].dirbits & 1 ? -m : 0);
		int nz = z + dz;
		ColResult cr;
		int done = 0, nland = 0, land[COL_MAX];
		if (nz < 0) cr.zf = 0; else collision_test(&wk->world, x + dx, y + dy, nz, xs, ys, zs, t, 0, &cr);
		if (cr.zf) {
			x += dx; y += dy; z = nz;
			if (rv->fly[k].hi < 18) rv->fly[k].hi++;
			if (rv->fly[k].zi < 40) rv->fly[k].zi++;
			if (nz == 0 && dz < 0) done = 1;
		} else {
			if (nz >= 0) { nland = cr.n; for (int q = 0; q < cr.n; q++) land[q] = cr.ids[q]; }
			if (rv->fly[k].fall && rv->fly[k].zi == 0) done = 1;             /* blocked at the very first entry of the fall path (the pointer equals FB8E): it rests here (16E74) */
			else {
				rv->fly[k].fall = 1; rv->fly[k].zi = 0; rv->fly[k].hi = 18;     /* 16E5D: the horizontal path is dropped and FB8E restarts */
				dz = zfall[0]; nz = z + dz;
				if (nz < 0) cr.zf = 0; else collision_test(&wk->world, x, y, nz, xs, ys, zs, t, 0, &cr);
				if (cr.zf) { z = nz; rv->fly[k].zi = 1; if (nz == 0) done = 1; }
				else done = 1;
			}
		}
		rv->vm.pos[3 * t] = (uint8_t)x; rv->vm.pos[3 * t + 1] = (uint8_t)y; rv->vm.pos[3 * t + 2] = (uint8_t)z;
		if (g_snd_log) fprintf(stderr, "FLY %d at %d,%d,%d dz %d%s\n", t, x, y, z, dz, done ? " landed" : "");
		rv->pending_rebuild = 1;
		if (done && z == 0) {                                            /* 1000:16E5D..16EB1: the fall reaches the floor (z becomes 0): event 11 (0Bh) for the thing (level 1 room 53's falling things, level 2 room 86's) */
			rv->vm.queue_n = 0;
			acl_thing_event(&rv->vm, &rv->mp2, t, 11, 0);
			for (int w = 0; w < rv->vm.queue_n; w++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[w].type, rv->vm.queue[w].arg);
			rv->pending_rebuild |= rv_changed(rv);
			roomview_flush_messages(rv);
		}
		if (done) { rv->fly[k] = rv->fly[--rv->nfly]; k--; }
		for (int q = 0; q < nland; q++) {                                /* event 4 (04h) for each thing it came to rest on, argument = the landed thing (level 1's keyholes: handler operand = the key's id) */
			if (land[q] <= 0 || land[q] >= ACL_THINGS || land[q] == t) continue;
			rv->vm.queue_n = 0;
			acl_thing_event(&rv->vm, &rv->mp2, land[q], 4, t);
			for (int w = 0; w < rv->vm.queue_n; w++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[w].type, rv->vm.queue[w].arg);
			rv->pending_rebuild |= rv_changed(rv);
			roomview_flush_messages(rv);
		}
	}
}

/* one game step in the rucksack view: Left/Right move the highlight (past the ends: the previous / next carried thing), End or Return runs the verb, Space leaves */
/* verb 9, drink (1000:32B5): event 5 for the potion, sound 0Ah, the identified flag A[3] bit 0 is cleared (unless bit 1), the doses A[2] drop
   (at 0 the potion is gone and leaves the hand); a bad potion (A[3] bit 4) costs A[1] energy with message 34, otherwise the TAD routine
   table[A[0]] acts: potions 0 (STAMINA), 1 (SUPER FAST), 2 (GIANT JUMP), 3 (ALCOHOL) and the shields 6 / 8 / 11 / 12 are ported (effect timers 23h / 21h / 20h); the others are not (nothing consumed). */
static void drink_potion(RoomView *rv, int t) {
	const ObjSprite *sp = thing_sprite(rv, t);
	if (!sp || sp->header[0x16] != 2) return;
	uint8_t *a = rv->vm.tail[t];
	int bad = (a[3] & 4) != 0;
	int shield = a[0] == 6 ? 0 : a[0] == 8 ? 1 : a[0] == 11 ? 2 : a[0] == 12 ? 3 : -1;
	if (!bad && a[0] > 3 && shield < 0 && a[0] != 14 && a[0] != 15 && a[0] != 16 && a[0] != 4 && a[0] != 5 && a[0] != 7 && a[0] != 9 && a[0] != 10 && a[0] != 13 && a[0] != 17 && !(rv->use_tata && (a[0] == 18 || a[0] == 19 || a[0] == 20 || a[0] == 21 || a[0] == 22))) return;
	rv->vm.queue_n = 0;
	acl_thing_event(&rv->vm, &rv->mp2, t, 5, 0);
	for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
	snd_play(0x0A);
	if (!(a[3] & 2)) a[3] &= (uint8_t)~1;
	a[2]--;
	if ((int8_t)a[2] <= 0) {
		rv->vm.flags[t] |= 0x80;
		if (rv->held == t) rv->held = 0;
	}
	if (bad) {                                                        /* 13352: `neg dx; call 181C8` takes ax, which still holds the thing's A-struct offset rec[0Ch] (the dispatcher's `mov al, [si+0Ch]`): a bad potion HEALS by that many points (an original bug, seen in the emulator: 100 -> 122) */
		int len; const uint8_t *rec = mp2_thing(&rv->mp2, t, &len);
		acl_energy_change(&rv->vm, rec && len > 12 ? rec[12] : 0);
		if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x22;
	}
	else if (shield >= 0) { rv->vm.g[0x11B] |= (uint8_t)(1 << shield); rv->vm.g[0x11C] = a[1]; if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x2C; }   /* shot / fire / ice / magic shield: ds:6AB bit, power ds:6AC, "THE GODS PROTECT YOU" */
	else if (a[0] == 0) acl_energy_change(&rv->vm, a[1]);
	else if (a[0] == 4) { rv->vm.g[0x119] = a[1]; if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x1E; }                 /* POISON (0753): ds:6A9 = power, message 30; nothing ticks it without the timers of op 5Ah */
	else if (a[0] == 7) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x2B; }                                     /* WATER (076A): "YOU FEEL REFRESHED" */
	else if (a[0] == 9) { int d = rv->vm.energy_max / 2 - rv->vm.energy; if (d >= 0) acl_energy_change(&rv->vm, d); }       /* CURE (0700): up to half of the maximum energy */
	else if (a[0] == 13) { if (rv->vm.g[0x119]) { rv->vm.g[0x119] = 0; if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x2D; } }   /* CURE POISON (07A2 -> 0CAE) */
	else if (a[0] == 17) acl_energy_change(&rv->vm, -32000);                                                                /* SUICIDE (07C4): energy -0x8300 */
	else if (a[0] == 14) { rv->vm.g[0x11B] = 0x7F; acl_set_timer(&rv->vm, 0x27, a[1]); }                                      /* TOTAL SHIELD (07AF): every shield bit, then as IMMORTAL */
	else if (a[0] == 15) acl_set_timer(&rv->vm, 0x27, a[1]);                                                                /* IMMORTAL (07B4): effect 27h: the energy does not change */
	else if (a[0] == 16) acl_set_timer(&rv->vm, 0x28, a[1]);                                                                /* STRENGTH (07B9): ds:59C = 0C7h (the push strength) for effect 28h */
	else if (rv->use_tata && a[0] == 19) rv->vm.score += 10u * a[1];                                                       /* WISDOM (Payoff 07C3): the experience ds:24C += 10 * power */
	else if (rv->use_tata && a[0] == 20) { uint32_t d = 10u * a[1]; rv->vm.score = rv->vm.score > d ? rv->vm.score - d : 0; }   /* STUPIDITY: -= 10 * power, not below 0 */
	else if (rv->use_tata && a[0] == 22) acl_set_timer(&rv->vm, 0x29, a[1]);                                                /* WEAKEN (Payoff): ds:59C = 0Ah (push strength) for effect 29h */
	else if (rv->use_tata && a[0] == 21) acl_set_timer(&rv->vm, 7, a[1]);                                                /* LEAPFROG (Payoff 0807): the routine's flag and timer 7 := A[1]; the timer routine 0DB6 only writes ds:6DC, which nothing reads (the forced jump is never wired up in the shipped game) */
	else if (rv->use_tata && a[0] == 18) { }                                                                                /* OIL: an empty routine (the scripts use it) */
	else if (a[0] == 5 || a[0] == 10) { }                                                                                   /* the blank potion and SLOW: empty routines */
	else acl_set_timer(&rv->vm, a[0] == 1 ? 0x23 : a[0] == 2 ? 0x21 : 0x20, a[1]);   /* super fast / giant jump / alcohol: effect timers 23h / 21h / 20h for A[1] units of 17 steps */
	rv->pending_rebuild |= rv_changed(rv);
	roomview_flush_messages(rv);
}
/* the whole-rucksack grid (1000:125CF): 4 items per row, at most 4 rows visible; the cursor starts at the thing being looked at */
static void grid_begin(RoomView *rv) {
	int l[64], n = ruck_list(rv, l, 64), k = 0;
	for (int i = 0; i < n; i++) if (l[i] == rv->touch_thing) k = i;
	rv->gx = k % 4; rv->grow = k / 4;
	int last = n > 0 ? (n - 1) / 4 : 0;
	rv->gscroll = last > 3 && rv->grow > 3 ? rv->grow - 3 : 0;
	rv->gvis = rv->grow - rv->gscroll;
}
/* one cursor move of 125CF (key bits 1 up, 2 down, 4 left, 8 right); transcribed from the code, including its wrap-around quirks: Up / Down past the ends of a grid of more than
   16 things leave the visible row and the absolute row out of step, and a wrap to the first row also clamps the column to the last row's width */
static void grid_nav(RoomView *rv, int key, int n) {
	ruckgrid_nav(key, n, &rv->gx, &rv->grow, &rv->gvis, &rv->gscroll);
	int l[64], m = ruck_list(rv, l, 64), idx = rv->grow * 4 + rv->gx;
	if (idx >= 0 && idx < m) rv->touch_thing = l[idx];
}
static void ruck_step(Walker *wk, RoomView *rv, int keys, int fire_edge, int space_edge) {
	if (!(keys & 0x80)) rv->menu_released = 1;
	if (rv->ruck_delay > 0) rv->ruck_delay--;
	if (space_edge) { rv->grid_open = 0; ruck_close(rv); return; }
	if (rv->grid_open) {                                              /* the strip of carried things: Left / Right move the frame, fire selects the thing and returns to its verbs */
		if (fire_edge) {
			int sel = rv->touch_thing;
			rv->grid_open = 0;
			for (int i = 0; i < 4; i++) rv->msgs[i].active = 0;
			rv->look_pic = 0;
			ruck_show(rv, sel);
			snd_play(0x37);
			return;
		}
		if (rv->ruck_delay == 0 && (keys & 15)) {
			int l[64], n = ruck_list(rv, l, 64);
			if (n > 0) grid_nav(rv, keys & 15, n);
			rv->ruck_delay = 13;
			snd_play(2);
		}
		return;
	}
	if (fire_edge && rv->menu_released && rv->nverbs > 0) {
		int verb = rv->verbs[rv->menu_sel], t = rv->touch_thing;
		snd_play(0x37);
		if (g_snd_log) fprintf(stderr, "RUCK verb %d sel %d of %d\n", verb, rv->menu_sel, rv->nverbs);
		if (verb == 6) ruck_close(rv);
		else if (verb == 1 && t >= 0 && t < ACL_THINGS) {               /* drop: back into the room at the player's position */
			/* 1000:6C72: the spot in front of the player in the eight facings (front_spot); a blocked spot refuses the drop */
			int dxp, dyp, z;
			if (!front_spot_h(wk, rv, t, &dxp, &dyp, &z, 21)) {                 /* 16C72 with cl = 0: z = the player's z + 21 (= 29/2 + 29/4: checked by calling the routine in the emulator for cl = 0, 1, 2, 3, E2h) */
				if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 27;     /* THERE IS NO ROOM TO PUT THIS OBJECT IN */
				roomview_flush_messages(rv);
				return;
			}
			if (g_snd_log) fprintf(stderr, "DROP %d at %d,%d z %d (player %d,%d,%d)\n", t, dxp, dyp, z, wk->x1, wk->y1, wk->z);
			rv->vm.pos[3 * t] = (uint8_t)dxp; rv->vm.pos[3 * t + 1] = (uint8_t)dyp; rv->vm.pos[3 * t + 2] = (uint8_t)z;
			rv->vm.room[t] = (uint8_t)rv->room;
			rv->loose[t] = 1;
			rv->pending_rebuild = 1;
			if (rv->held == t) rv->held = 0;
			rv->vm.queue_n = 0;                                          /* 1000:2F0E: event 21 (15h) for the dropped thing */
			acl_thing_event(&rv->vm, &rv->mp2, t, 21, 0);
			for (int q = 0; q < rv->vm.queue_n; q++)
				acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
			rv->pending_rebuild |= rv_changed(rv);
			roomview_flush_messages(rv);
			ruck_close(rv);
		} else if (verb == 8) {                                           /* the diary's class verb while carried: the strip with "THIS IS A DIARY" (emulator) */
			const ObjSprite *dsp = thing_sprite(rv, t);
			int nm = dsp ? (dsp->header[10] << 8) | dsp->header[11] : 0;
			char tx[200];
			snprintf(tx, sizeof(tx), "%s%s", lng_str(rv, 5), nm < 0x8000 ? lng_str(rv, nm) : "");
			msg_show_text(rv, tx);
			rv->grid_open = 1; rv->touch_thing = t; grid_begin(rv); grid_verbs(rv);
		} else if ((verb == 15 || verb == 12 || verb == 16) && rv->ruck_target > 0) {
			int tg = rv->ruck_target;
			const ObjSprite *ts = thing_sprite(rv, tg);
			int tc = ts ? ts->header[0x16] : -1;
			rv->vm.queue_n = 0;
			if (verb == 15) {                                           /* 1000:344D: event 26 (1Ah) for the touched thing, argument = the carried thing; the carried thing leaves the rucksack (13509) */
				acl_thing_event(&rv->vm, &rv->mp2, tg, 26, t);
				rv->vm.flags[t] |= 0x80; if (rv->held == t) rv->held = 0;
			} else if (verb == 12) {                                    /* 1000:3381: put into a container (class 8: its content word A[0..1] must be empty; the lid closes) or hand over to a class 0Bh thing; sound 1Eh, event 18 (12h) with the carried thing */
				uint8_t *ta = rv->vm.tail[tg];
				if (tc == 8 && (ta[0] || ta[1])) { if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x1B; }
				else {
					if (tc == 8) { ta[0] = (uint8_t)(t >> 8); ta[1] = (uint8_t)t; rv->vm.flags[t] |= 0x80; if (rv->held == t) rv->held = 0; ta[4] &= (uint8_t)~1; }
					snd_play(0x1E);
					acl_thing_event(&rv->vm, &rv->mp2, tg, 18, t);
				}
			} else {                                                    /* 1000:2E40: event 1 for the touched thing, then the spell routine with ds:6A8 = 1 (the touched thing is the target) */
				acl_thing_event(&rv->vm, &rv->mp2, tg, 1, t);
				for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
				rv->vm.queue_n = 0;
				const ObjSprite *cs = thing_sprite(rv, t);
				if (cs && cs->header[0x16] == 1 && !(rv->vm.flags[t] & 0x80)) { rv->cast_target = tg; cast_spell_item(wk, rv, t); rv->cast_target = 0; }
			}
			for (int q = 0; q < rv->vm.queue_n; q++)
				acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
			rv->pending_rebuild |= rv_changed(rv);
			roomview_flush_messages(rv);
			ruck_close(rv);
		} else if (verb == 9) {
			drink_potion(rv, t);
			if (rv->vm.flags[t] & 0x80) ruck_close(rv);
		} else if (verb == 11 || verb == 13 || verb == 4 || verb == 7) {
			menu_choose(rv, verb);
			if (verb == 11) { rv->grid_open = 1; rv->touch_thing = t; grid_begin(rv); grid_verbs(rv); }
		}
		return;
	}
	if (rv->ruck_delay == 0 && (keys & 12)) {
		int l[64], n = ruck_list(rv, l, 64), k = 0;
		for (int i = 0; i < n; i++) if (l[i] == rv->touch_thing) k = i;
		if (keys & 8) {
			if (rv->menu_sel + 1 < rv->nverbs) rv->menu_sel++;
			else if (n > 1) ruck_show(rv, l[(k + 1) % n]);
		} else {
			if (rv->menu_sel > 0) rv->menu_sel--;
			else if (n > 1) { ruck_show(rv, l[(k + n - 1) % n]); rv->menu_sel = rv->nverbs - 1; }
		}
		rv->ruck_delay = 13;
		snd_play(2);
	}
}

/* one game step with the verb menu open (1000:2A33): Left/Right move the highlight (every 13 steps while held), End chooses, Space/End-less cancel */
static void menu_step(RoomView *rv, int keys, int end_edge) {
	if (!(keys & 0x80)) rv->menu_released = 1;
	if (rv->menu_delay > 0) rv->menu_delay--;
	if (end_edge && rv->menu_released) {
		int verb = rv->verbs[rv->menu_sel];
		rv->menu_open = 0;
		snd_play(0x37);                                           /* 1000:2BD3: every chosen verb */
		menu_choose(rv, verb);
		return;
	}
	if (rv->menu_delay == 0 && rv->nverbs > 0) {
		if (keys & 8) { rv->menu_sel = (rv->menu_sel + 1) % rv->nverbs; rv->menu_delay = 13; snd_play(2); }          /* 1000:2B60: highlight click */
		else if (keys & 4) { rv->menu_sel = (rv->menu_sel + rv->nverbs - 1) % rv->nverbs; rv->menu_delay = 13; snd_play(2); }
	}
}

/* op 37: put the player into `room` at (x + rec[1], y + rec[3], z) like 1000:7F3C; z grows until the spot is free */
static void walker_teleport(Walker *wk, RoomView *rv, int room, int x, int y, int z) {
	wk->grab = 0;
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

/* ---- quick save / load (F5 / F9, file cadaver.sav in the working directory; the original's own save format, data\\mike_save, is not decoded): the whole script VM plus the
 * player's place and the clock ---- */
typedef struct {
	char magic[4];
	int32_t version, level, room, tata, x1, y1, z, dir, day;
	uint32_t clock_ticks;
	int32_t held, last_item, save_price;
	uint8_t visited[128];
	int32_t vm_size;
} SaveHead;
static uint8_t *g_sav;               /* loaded save waiting for the level to be (re)loaded */
static size_t g_sav_len;
static int g_sav_place;              /* ... and for the player to be placed */
static int game_save(const Walker *wk, const RoomView *rv, int slot) {
	if (!rv->ok) return -1;
	char fn[32];
	if (slot < 0) snprintf(fn, sizeof(fn), "cadaver.sav"); else snprintf(fn, sizeof(fn), "cadaver%d.sav", slot);
	FILE *f = fopen(fn, "wb");
	if (!f) return -1;
	SaveHead h;
	memset(&h, 0, sizeof(h));
	memcpy(h.magic, "CSAV", 4);
	h.version = 2; h.save_price = rv->save_price; h.level = rv->level; h.room = rv->room; h.tata = rv->use_tata;
	h.x1 = wk->x1; h.y1 = wk->y1; h.z = wk->z; h.dir = wk->dir; h.day = rv->day; h.clock_ticks = rv->clock_ticks;
	h.held = rv->held; h.last_item = rv->last_item;
	memcpy(h.visited, rv->visited, sizeof(h.visited));
	h.vm_size = (int32_t)sizeof(AclVM);
	fwrite(&h, sizeof(h), 1, f);
	fwrite(&rv->vm, sizeof(AclVM), 1, f);
	fclose(f);
	return 0;
}
/* reads the file, tears the level down and lets the next draw reload it; the VM is restored by roomview_load_level */
static int game_load(RoomView *rv, int slot) {
	char fn[32];
	if (slot < 0) snprintf(fn, sizeof(fn), "cadaver.sav"); else snprintf(fn, sizeof(fn), "cadaver%d.sav", slot);
	FILE *f = fopen(fn, "rb");
	if (!f) return -1;
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	SaveHead h;
	if (n < (long)sizeof(h) || fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "CSAV", 4) || h.version != 2 || h.vm_size != (int32_t)sizeof(AclVM) || n != (long)(sizeof(h) + sizeof(AclVM))) { fclose(f); return -1; }
	free(g_sav);
	g_sav = malloc((size_t)n);
	fseek(f, 0, SEEK_SET);
	if (!g_sav || fread(g_sav, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(g_sav); g_sav = NULL; return -1; }
	fclose(f);
	g_sav_len = (size_t)n;
	roomview_unload(rv);
	rv->level = h.level; rv->room = h.room; rv->use_tata = h.tata;
	rv->restarting = 1;
	rv->dead_timer = 0;
	g_walk.world_key = 0; g_walk.placed = 0;
	return 0;
}
/* called by roomview_load_level after the level is loaded */
static void game_load_apply(RoomView *rv) {
	if (!g_sav) return;
	const SaveHead *h = (const SaveHead *)g_sav;
	AclVM keep = rv->vm;
	memcpy(&rv->vm, g_sav + sizeof(SaveHead), sizeof(AclVM));
	rv->vm.clone_ok = 1;
	rv->vm.mp2 = keep.mp2; rv->vm.sound_stop = keep.sound_stop; rv->vm.rand_range = keep.rand_range; rv->vm.sound = keep.sound; rv->vm.user = keep.user;
	rv->vm.msg_n = 0; rv->vm.queue_n = 0; rv->vm.tev_n = 0; rv->vm.go.pending = 0; rv->vm.next_level = 0;
	acl_restore_clones(&rv->vm);
	memcpy(rv->visited, h->visited, sizeof(rv->visited));
	rv->day = h->day; rv->clock_ticks = h->clock_ticks; rv->held = h->held; rv->last_item = h->last_item; rv->save_price = h->save_price;
	rv->room = h->room;
	g_sav_place = 1;
}

static void walker_tick(Walker *wk, RoomView *rv, int keys) {
	if (rv->restarting) return;
	roomview_tick(rv);
	wk->tick++;
	if (wk->tick == 8 && getenv("CAD_DEATH")) { int t = atoi(getenv("CAD_DEATH")); rv->vm.queue_n = 0; acl_thing_event(&rv->vm, &rv->mp2, t, 23, 0); for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg); rv->pending_rebuild |= rv_changed(rv); }       /* debugging aid: event 23 (death) for this thing */
	if (wk->tick == 8 && getenv("CAD_ENDING")) { rv->vm.game_end = 1; if (rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 77; }       /* debugging aid: the ending of level 5 (message 77, then the status box) without playing to it */
	rv->vm.held = rv->held;
	rv->vm.px = wk->x1; rv->vm.py = wk->y1; rv->vm.pz = wk->z;
	if (g_dbg_shot && (wk->tick % 4) == 0) { int st = atoi(getenv("CAD_SHOT")); fprintf(stderr, "S%d tick %d pos %d,%d,%d flags %02x room %d mode %d vel %d,%d A2 %d A0 %d player %d,%d dir %d\n", st, wk->tick, rv->vm.pos[3*st], rv->vm.pos[3*st+1], rv->vm.pos[3*st+2], rv->vm.flags[st], rv->vm.room[st], rv->vm.mstate[st][0], (int8_t)rv->vm.mstate[st][3], (int8_t)rv->vm.mstate[st][4], rv->vm.tail[st][2], rv->vm.tail[st][0], wk->x1, wk->y1, wk->dir); }
	if (g_dbg_vars) { static uint8_t pg[0x190]; for (int i = 0; i < 0x190; i++) if (pg[i] != rv->vm.g[i]) { fprintf(stderr, "G tick %d g[%02x] %02x -> %02x\n", wk->tick, i, pg[i], rv->vm.g[i]); pg[i] = rv->vm.g[i]; } }
	if (g_snd_log) { static int pf = -1, pr = -1; if (rv->vm.flags[467] != pf || rv->vm.room[467] != pr) { fprintf(stderr, "T467 unsup %d last %02x tick %d flags %02x room %d pos %d,%d,%d\n", rv->tad.unsupported, rv->tad.last_unsupported, wk->tick, rv->vm.flags[467], rv->vm.room[467], rv->vm.pos[3*467], rv->vm.pos[3*467+1], rv->vm.pos[3*467+2]); pf = rv->vm.flags[467]; pr = rv->vm.room[467]; } }
	if (rv->look_pic) { int any = 0; for (int i = 0; i < 4; i++) any |= rv->msgs[i].active; if (!any) rv->look_pic = 0; }
	if (rv->pending_rebuild && wk->world_key == room_key(rv)) walker_world(wk, rv);   /* things changed: rebuild objects and collision world now, not at the next draw */
	if ((wk->tick & 1) == 0 && wk->world_key == room_key(rv) && !rv->menu_open && rv->dead_timer == 0)
		creatures_step(wk, rv);
	if ((wk->tick & 7) == 0 && wk->world_key == room_key(rv) && !rv->menu_open && rv->dead_timer == 0) gravity_scan(wk, rv);
	if ((wk->tick & 1) == 0 && rv->nfly && wk->world_key == room_key(rv) && !rv->menu_open && rv->dead_timer == 0)
		fly_step(wk, rv);
	if (wk->world_key != room_key(rv)) {
		walker_world(wk, rv);
		walker_place_default(wk, rv);
		if (g_sav_place && g_sav) {                         /* a loaded game: the saved place */
			const SaveHead *h = (const SaveHead *)g_sav;
			wk->x1 = h->x1; wk->y1 = h->y1; wk->z = h->z; wk->dir = h->dir;
			free(g_sav); g_sav = NULL; g_sav_place = 0;
			walker_world(wk, rv);
		}
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
		if (!tata && nl > 5 && g_payoff) { rv->use_tata = 1; rv->level = 1; }     /* the Payoff picks up where Cadaver ends */
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
	if (rv->map_open) { if ((wk->tick & 1) == 0) map_step(rv, keys); return; }
	if (wk->tick & 1)
		return;
	{
		int end = (keys & 0x80) != 0, edge = end && !rv->end_prev;
		rv->end_prev = end;
		if (rv->touch_age < 1000) rv->touch_age++;
		int sp = (keys & 0x10) != 0, spe = sp && !rv->space_prev, rt = (keys & 0x20) != 0, rte = rt && !rv->ret_prev;
		rv->space_prev = sp; rv->ret_prev = rt;
		{                                                                 /* H: hold / put away the last handled carried thing (1000:03CD) */
			int h = (keys & 0x100) != 0, he = h && !rv->h_prev;
			rv->h_prev = h;
			if (he && !rv->ruck_open && !rv->menu_open) {              /* 1000:03CD: with a held thing H puts it away and remembers it in ds:58A; with none it holds ds:58A again when that is still carried (a thing taken or held through the rucksack is not remembered: checked in the emulator, 58A stays 0 after a pickup and H does nothing) */
				if (rv->held) { rv->h_item = rv->held; rv->held = 0; }
				else if (rv->h_item > 0 && rv->h_item < ACL_THINGS && rv->vm.room[rv->h_item] == 0xFF && !(rv->vm.flags[rv->h_item] & 0x80)) {
					rv->held = rv->h_item;
					rv->vm.queue_n = 0;
					acl_thing_event(&rv->vm, &rv->mp2, rv->held, 27, 0);
					for (int q = 0; q < rv->vm.queue_n; q++) acl_room_event(&rv->vm, &rv->mp2, rv->room, rv->vm.queue[q].type, rv->vm.queue[q].arg);
				}
			}
		}
		if (rv->vm.game_end && !rv->ending) { rv->ending = 1; rv->show_status = 1; }                  /* level 5's ending routine reached service 3Ah: the original shows the message scroll and the status box and waits for a key before it exits to DOS */
		if (rv->ending && (edge || spe || rte)) { g_quit_req = 1; return; }
		if (rv->ending) return;
		if (rv->show_status && (edge || spe || rte) ) { rv->show_status = 0; return; }
		if (rv->ruck_open) { ruck_step(wk, rv, keys, edge || rte, spe); return; }
		if ((spe || rte) && !rv->menu_open) { ruck_open(rv); if (rv->ruck_open) return; }
		if (rv->menu_open) { menu_step(rv, keys, edge); return; }
		if (edge && rv->nverbs && rv->touch_age < 12 && !(keys & 15) ) {   /* End tapped right after touching an object (menu2 trace) */
			rv->menu_open = 1; rv->menu_sel = 0; rv->menu_delay = 0; rv->menu_released = 0;
			return;
		}
		if (rv->held && rv->held < ACL_THINGS && rv->dead_timer == 0) {      /* fire with a held thing: throw it instead of jumping; a held spell / weapon fires while End stays down, once the cooldown ds:629 is 0 (1000:0687 .. 0701) */
			const ObjSprite *hs = thing_sprite(rv, rv->held);
			int hc = hs ? hs->header[0x16] : -1;
			if ((hc == 1 || hc == 4) ? ((keys & 0x80) && rv->fire_cool == 0) : edge) throw_held(wk, rv);
		}
	}
	{
		/* Jump (emulator, level 1 room 0, End held): after 3 steps z follows +5 5 4 4 4 4 3 3 2 1 then -1 2 3 3 4 4 4 4 5 5 (peak 35, 20 steps), then 2 steps on the ground and
		 * the next jump while End is still held; the horizontal move continues at one unit per step. Landing / ceilings: the collision test at the new z. */
		static const int8_t zprof_n[20] = {5, 5, 4, 4, 4, 4, 3, 3, 2, 1, -1, -2, -3, -3, -4, -4, -4, -4, -5, -5};
		static const int8_t zprof_g[20] = {8, 8, 6, 6, 6, 6, 4, 4, 2, 1, -1, -2, -4, -4, -6, -6, -6, -6, -8, -8};   /* giant jump (effect 21h): the table at ds:D8 + 1Ch */
		const int8_t *zprof = (wk->jumping ? wk->giant : acl_timer_active(&rv->vm, 0x21)) ? zprof_g : zprof_n;
		ColResult jc;
		int end = (keys & 0x80) != 0;
		wk->jhold = end ? wk->jhold + 1 : 0;
		int below_free = 0;
		if (wk->z > 0) { collision_test(&wk->world, wk->x1, wk->y1, wk->z - 1, PL_XS, PL_YS, PL_ZS, -1, 1, &jc); below_free = jc.zf; }
		if (!wk->jumping) {
			if (wk->jcool > 0) wk->jcool--;
			else if (end && wk->jhold >= 3 && !below_free && !rv->held) { wk->jumping = 1; wk->jstep = 0; wk->fall = 0; wk->giant = acl_timer_active(&rv->vm, 0x21); snd_play(0x0E); }          /* 1000:2DF1 */
		}
		if (wk->jumping) {
			int nz = wk->z + zprof[wk->jstep];
			if (nz < 0) nz = 0;
			collision_test(&wk->world, wk->x1, wk->y1, nz, PL_XS, PL_YS, PL_ZS, -1, 1, &jc);
			if (jc.zf) { wk->z = nz; wk->jstep++; }
			else if (wk->jstep < 10) wk->jstep = 10;                  /* hit a ceiling: start falling */
			else { wk->jumping = 0; wk->jcool = 2; }                  /* landed on something */
			if (wk->jstep >= 20) { wk->jumping = 0; wk->jcool = 2; snd_play(0x18); }     /* landing: 1000:0548 */
		} else if (below_free) {                                       /* walked off an object: fall */
			int d = 1 + wk->fall++; if (d > 5) d = 5;
			int nz = wk->z - d; if (nz < 0) nz = 0;
			collision_test(&wk->world, wk->x1, wk->y1, nz, PL_XS, PL_YS, PL_ZS, -1, 1, &jc);
			if (jc.zf) wk->z = nz; else wk->fall = 0;
		} else wk->fall = 0;
		if (g_snd_log && (wk->jumping || wk->z)) fprintf(stderr, "Z %d\n", wk->z);
	}
	int dx = 0, dy = 0, dir = -1;
	if (g_four_dir && (keys & 12) && (keys & 3)) keys &= ~3;
	if (keys & 8) { dir = DIR_RIGHT; dx = 1; }          /* both axes may be held (diagonals): one combined move, no sliding (emulator: both stop together) */
	else if (keys & 4) { dir = DIR_LEFT; dx = -1; }
	if (keys & 2) { dy = 1; if (dir < 0) dir = DIR_DOWN; }
	else if (keys & 1) { dy = -1; if (dir < 0) dir = DIR_UP; }
	if (dir < 0) {
		wk->moving = 0; wk->anim = 0;
		if (rv->push_snd) { snd_stop(rv->push_snd); rv->push_snd = 0; }
		return;
	}
	if (acl_timer_active(&rv->vm, 0x23)) { dx *= 2; dy *= 2; }          /* super fast (effect 23h: ds:60E = ds:60F = 2) */
	if (acl_timer_active(&rv->vm, 0x20)) { dx = -dx; dy = -dy; }         /* alcohol (effect 20h, ds:64B): the move is reversed (1000:10AF6) */
	wk->dir = dir;
	wk->face = keys & 15;
	wk->moving = 1;
	wk->world.dirbits = (keys & 3) ? 1 : 0;
	int gw, gh, gx, gy;
	if (!roomview_geom(rv, &gw, &gh, &gx, &gy))
		return;
	ColResult cr;
	collision_test(&wk->world, wk->x1 + dx, wk->y1 + dy, wk->z, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
	if (cr.count == 0xFE) {
		int d = cr.exit_id;
		int t = door_target(rv, d);
		if (t < 0 || d < 0 || d >= 400) return;
		/* 1000:089C..0963: the lock word of the door record (+2, `AclVM.doorw`): 0 = open; FFFF = shut for good (the message 30h when the flag byte +7 has bit 6); anything else is the id of a thing the player must carry
		   (18726): without it nothing happens, with it the door opens (sound 12h) and the lock goes for good when byte +7 has bit 1 (the key is used up: removed and destroyed, 13F42 / 162F0) or bit 2 (the lock only) */
		unsigned w = rv->vm.doorw[d];
		if (w) {
			const uint8_t *dr = rv->mp2.sec[1].rec[d];
			int dfl = dr && rv->mp2.sec[1].len[d] > 7 ? dr[7] : 0;
			if (w == 0xFFFF) {
				if ((dfl & 0x40) && rv->vm.msg_n < 16) rv->vm.msg[rv->vm.msg_n++] = 0x30;
				return;
			}
			if (w >= ACL_THINGS || !rv->vm.present[w] || rv->vm.room[w] != 0xFF || (rv->vm.flags[w] & 0x80)) return;
			if (dfl & 2) { rv->vm.flags[w] |= 0x80; if (rv->held == (int)w) rv->held = 0; }
			if (dfl & 6) rv->vm.doorw[d] = 0;
			if (rv->vm.sound) rv->vm.sound(0x12, rv->vm.user);
		}
		walker_enter(wk, rv, t, d);
		return;
	}
	if (!cr.zf && cr.count != 0xFF && cr.count != 0xFE)
		walker_touch(wk, rv, &cr, keys);
	if (!cr.zf && cr.count != 0xFF && cr.flag == 0xFD && cr.n >= 1 && cr.n <= 2) {      /* 1000:05F1: blocked by at most two objects, a solid one among them: the first one, when its sprite header byte 0Ch has bit 3, can be climbed if its top + 1 is less than 20 above the player and the spot is free there */
		const RoomObj *co = NULL;
		for (int i = 0; i < rv->ro.n; i++) if (rv->ro.o[i].thing == cr.ids[0]) { co = &rv->ro.o[i]; break; }
		if (co && (co->sp->header[12] & 8)) {
			int nz = (int8_t)co->czt + 1;
			if (((nz - wk->z) & 0xFF) < 0x14) {
				ColResult c2;
				collision_test(&wk->world, wk->x1 + dx, wk->y1 + dy, nz, PL_XS, PL_YS, PL_ZS, -1, 1, &c2);
				if (c2.zf) { if (g_snd_log) fprintf(stderr, "CLIMB onto %d: z %d -> %d at %d,%d by %d,%d\n", co->thing, wk->z, nz, wk->x1 + dx, wk->y1 + dy, dx, dy); wk->x1 += dx; wk->y1 += dy; wk->z = nz; wk->fall = 0; return; }
			}
		} else if (acl_timer_active(&rv->vm, 0x23) && !(dx & 1) && !(dy & 1) && (dx || dy)) {   /* 063B: the doubled step (super fast) is retried halved */
			dx /= 2; dy /= 2;
			collision_test(&wk->world, wk->x1 + dx, wk->y1 + dy, wk->z, PL_XS, PL_YS, PL_ZS, -1, 1, &cr);
		}
	}
	static PushObj po[ROOMOBJS_MAX];
	for (int i = 0; i < rv->ro.n; i++) {
		const RoomObj *o = &rv->ro.o[i];
		po[i].x1 = o->x1; po[i].y1 = o->y1; po[i].z0 = o->z0;
		po[i].xs = o->xs; po[i].ys = o->ys; po[i].zs = o->zs; po[i].weight = o->weight;
	}
	int moved[PUSH_MAX], nm = 0;
	int strength = acl_timer_active(&rv->vm, 0x28) ? 0xC7 : acl_timer_active(&rv->vm, 0x29) ? 10 : 50;
	int st = PUSH_FREE, pulled = 0;
	if (keys & 0x80) wk->grab = 0;                                       /* 1000:10687: the fire key lets go */
	if (wk->grab) {
		/* 1000:10732: only when the step ahead is free; the player's box one step *behind* must touch the grabbed thing (116E0 + 16C3F, first entry not a wall) and the group that
		   blocks it (16570) moves by the step too, with the pushing strength; any failure drops the grab (1084E) */
		if (cr.zf) {
			ColResult pr;
			collision_test(&wk->world, wk->x1 - dx, wk->y1 - dy, wk->z, PL_XS, PL_YS, PL_ZS, -1, 1, &pr);
			int has = 0;
			if (!pr.zf && pr.count != 0xFF && pr.count != 0xFE && !(pr.flag & 0x80))
				for (int k = 0; k < pr.n; k++) if (pr.ids[k] == wk->grab - 1) has = 1;
			if (has) {
				st = push_move(&wk->world, po, wk->x1 - 2 * dx, wk->y1 - 2 * dy, wk->z, PL_XS, PL_YS, PL_ZS, -1, dx, dy, strength, moved, &nm);
				pulled = st == PUSH_MOVED && nm > 0;
				if (g_snd_log && pulled) fprintf(stderr, "PULL thing %d + %d more: player %d,%d by %d,%d\n", rv->ro.o[moved[0]].thing, nm - 1, wk->x1, wk->y1, dx, dy);
			}
		}
		if (!pulled) { wk->grab = 0; nm = 0; st = PUSH_FREE; }
	}
	if (!pulled) st = push_move(&wk->world, po, wk->x1, wk->y1, wk->z, PL_XS, PL_YS, PL_ZS, -1, dx, dy, strength, moved, &nm);
	wk->grab_rev = wk->grab != 0;
	if (wk->grab) { static const int opp[4] = {DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN}; wk->dir = opp[dir]; }   /* the player keeps facing the thing */
	if (st == PUSH_MOVED || st == PUSH_FREE) {
		if (g_snd_log && getenv("CAD_POSLOG")) fprintf(stderr, "POS tick %d %d,%d -> +%d,%d\n", wk->tick, wk->x1, wk->y1, dx, dy); wk->x1 += dx; wk->y1 += dy;
		for (int k = 0; k < nm; k++) {
			RoomObj *o = &rv->ro.o[moved[k]];
			o->x1 += dx; o->y1 += dy;
			roomobj_moved(o, gx, gy);
		}
		if (nm) walker_sync_boxes(wk, rv);
	}
	{
		int want = 0;
		if (st == PUSH_MOVED && nm > 0 && rv->ro.o[moved[0]].sp) {         /* 1000:121B0: looping scrape sound of the pushed object's class */
			int cls = rv->ro.o[moved[0]].sp->header[0x1D];
			want = cls < (int)sizeof(push_sound) ? push_sound[cls] : 0;
		}
		if (want != rv->push_snd) {
			if (rv->push_snd) snd_stop(rv->push_snd);
			if (want) snd_play(want);
			rv->push_snd = want;
		}
	}
	if ((wk->tick / 2) % 3 == 0) {
		wk->anim = (wk->anim + 1) & 3;
		if (wk->anim & 1) snd_play(0);                         /* the walking animation script's FC 00: a step every 6 game steps */
	}
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
	int f = g_walk.moving ? walk_frames[g_walk.dir][g_walk.grab_rev ? 3 - g_walk.anim : g_walk.anim] : idle_frame[g_walk.dir];
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
static int g_intro_ok;
static IntroScript *g_intro_script;   /* the story roll cut out of INTRO.EXE (NULL when it cannot be read: only the static title page is shown) */
static double g_intro_t;             /* seconds since the roll started */

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
	if (!g_intro_script && getenv("CAD_INTRO_T")) g_intro_t = atof(getenv("CAD_INTRO_T"));      /* debugging aid: start the roll this many seconds in */
	if (!g_intro_script) {
		uint8_t *x; size_t lx;
		if (gamefiles_read(g, "intro.exe", -1, &x, &lx) == 0) { g_intro_script = introscript_load(x, lx); free(x); }
		if (!g_intro_script) g_intro_script = (IntroScript *)(uintptr_t)1;      /* tried: none */
	}
	if (g_intro_script != (IntroScript *)(uintptr_t)1) intro_render_roll(scr, &g_intro_bg, &g_intro_font, &g_intro_flames, g_intro_script, g_intro_t);
	else intro_render_title(scr, &g_intro_bg, &g_intro_font, &g_intro_flames, g->id == GAME_PAYOFF, ((int)(g_intro_t * 70.086)) % 6);
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

/* The S / L keys (PAYOFF.DOC; 1000:BF92): S asks the price of a saved position (the larger of ds:246 and 50 * level - 45 gold; string 60 + the price, 62 "DO YOU WANT TO PAY. PRESS Y OR N.",
 * 61 when there is not enough money), Y pays (the next price rises by 6 * level) and asks for a slot 0-9; L asks for a slot and loads (free). The game stands still meanwhile.
 * The slots are the files cadaver0.sav ... cadaver9.sav (the original's disk format is not decoded). */
static int g_savemode;               /* 0 none, 1 waiting for Y / N, 2 slot to save, 3 slot to load */
static int g_save_pay;
static void savemode_text(const char *a, int n, const char *b) {
	char tx[200];
	if (n >= 0) snprintf(tx, sizeof(tx), "%s%d %s", a, n, b); else snprintf(tx, sizeof(tx), "%s", a);
	msg_show_text(&g_room, tx);
}
static void savemode_start(int save) {
	if (!save) { g_savemode = 3; g_paused = 1; snd_silence(); savemode_text("LOAD POSITION. PRESS 0 TO 9.", -1, ""); return; }
	int lv = g_room.level;
	int price = 50 * lv - 45;
	if (price < g_room.save_price) price = g_room.save_price;
	if ((int)g_room.vm.money < price) { savemode_text(lng_str(&g_room, 61), -1, ""); return; }
	g_save_pay = price; g_savemode = 1; g_paused = 1; snd_silence();
	savemode_text(lng_str(&g_room, 60), price, lng_str(&g_room, 62));
}
static void savemode_end(void) { g_savemode = 0; g_paused = 0; g_room.snd_key = 0; g_room.nroom_snd = 0; g_room.push_snd = 0; g_room.pending_rebuild = 1; for (int i = 0; i < 4; i++) g_room.msgs[i].active = 0; }
static void savemode_key(SDL_Keycode k) {
	if (k == SDLK_ESCAPE || (g_savemode == 1 && k == SDLK_n)) { savemode_end(); return; }
	if (g_savemode == 1 && k == SDLK_y) { g_room.vm.money -= (uint32_t)g_save_pay; g_room.save_price = g_save_pay + 6 * g_room.level; g_savemode = 2; savemode_text("SAVE POSITION. PRESS 0 TO 9.", -1, ""); return; }
	int d = k >= SDLK_0 && k <= SDLK_9 ? (int)(k - SDLK_0) : k >= SDLK_KP_1 && k <= SDLK_KP_9 ? (int)(k - SDLK_KP_1 + 1) : k == SDLK_KP_0 ? 0 : -1;
	if (d < 0) return;
	if (g_savemode == 2) { game_save(&g_walk, &g_room, d); savemode_end(); }
	else if (g_savemode == 3) { savemode_end(); game_load(&g_room, d); }
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
	int start_tata = 0;          /* --tata with --scene game: Space starts The Payoff directly */
	int scene_given = 0;
	int game_mode = 0;           /* --scene game: title scroll, then Space/Return starts level 1 in the walk scene */
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-g") && i + 1 < argc)
			dir = argv[++i];
		else if (!strcmp(argv[i], "--level") && i + 1 < argc)
			g_room.level = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--room") && i + 1 < argc)
			g_room.room = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--lang") && i + 1 < argc) g_lang = argv[++i];
		else if (!strcmp(argv[i], "--sound") && i + 1 < argc) g_sound_opt = argv[++i];
		else if (!strcmp(argv[i], "--tata"))
			start_tata = g_room.use_tata = 1;
		else if (!strcmp(argv[i], "--sim") && i + 1 < argc)
			sim = argv[++i];
		else if (!strcmp(argv[i], "--scroll") && i + 1 < argc)
			g_room.scroll_y = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
			shot = argv[++i];
		else if (!strcmp(argv[i], "--scene") && i + 1 < argc) {
			scene_given = 1;
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
	g_payoff = g.id == GAME_PAYOFF;

	static Screen scr;
	static uint32_t rgb[SCREEN_W * SCREEN_H];
	if (!scene_given && !sim && !shot) { game_mode = 1; scene = SCENE_INTRO; }      /* plain `cadaver -g dir` plays the game */
	if (load_scene(&g, scene, &scr) != 0)
		return 1;

	g_snd_log = getenv("CAD_SNDLOG") != NULL;
	g_dbg_zones = getenv("CAD_ZONES"); g_dbg_creat = getenv("CAD_CREAT"); g_dbg_shot = getenv("CAD_SHOT"); g_dbg_vars = getenv("CAD_VARS");
	{
		uint8_t *bin; size_t bl;
		int adlib = 0;                                     /* --sound adlib|beeper, else the first letter of CADAVER.INI's sound line */
		int sb = 0, rol = 0;
		if (g_sound_opt) { adlib = !strcmp(g_sound_opt, "adlib"); sb = !strcmp(g_sound_opt, "sblaster"); rol = !strcmp(g_sound_opt, "roland"); }
		else {
			uint8_t *ini; size_t il;
			if (gamefiles_read(&g, "cadaver.ini", -1, &ini, &il) == 0) { adlib = il > 0 && (ini[0] == 'A' || ini[0] == 'a'); sb = il > 0 && (ini[0] == 'S' || ini[0] == 's'); rol = il > 0 && (ini[0] == 'R' || ini[0] == 'r'); free(ini); }
		}
		if (gamefiles_read(&g, rol ? "roland.bin" : sb ? "sblaster.bin" : adlib ? "adlib.bin" : "beeper.bin", -1, &bin, &bl) == 0) {
			uint8_t *un = NULL; size_t ul = 0;
			if (bl > 4 && !memcmp(bin, "RNC\1", 4) && rnc1_unpack(bin, bl, &un, &ul) == 0) { free(bin); bin = un; bl = ul; }
			g_snd_ok = (rol ? beeper_init_roland(&g_snd, bin, bl, 44100) : beeper_init_kind(&g_snd, bin, bl, 44100, adlib || sb)) == 0;
			free(bin);
		}
	}
	if (SDL_Init(SDL_INIT_VIDEO | ((g_snd_ok && !sim && !shot) ? SDL_INIT_AUDIO : 0)) != 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return 1;
	}
	SDL_Window *win = SDL_CreateWindow(gamefiles_name(g.id), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                   960, 720, SDL_WINDOW_RESIZABLE);
	SDL_Renderer *ren = win ? SDL_CreateRenderer(win, -1, 0) : NULL;
	SDL_Texture *tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
	                                           SCREEN_W, SCREEN_H) : NULL;
	if (!tex) {
		fprintf(stderr, "SDL: %s\n", SDL_GetError());
		return 1;
	}
	/* the original's 320x200 picture was shown on a 4:3 screen (non-square pixels, x1.2 taller): the window keeps 4:3 */
	if (g_snd_ok && SDL_WasInit(SDL_INIT_AUDIO)) {
		SDL_AudioSpec want;
		SDL_zero(want);
		want.freq = 44100; want.format = AUDIO_S16SYS; want.channels = 1; want.samples = 1024; want.callback = snd_callback;
		g_adev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
		if (g_adev) SDL_PauseAudioDevice(g_adev, 0);
		else fprintf(stderr, "no audio: %s\n", SDL_GetError());
	}

	int running = 1, frames = 0;
	Scene prev_scene = scene;
	while (running) {
		if (scene != prev_scene) {                     /* the title tune (INTRO.EXE 10F78) plays while the title scroll is shown */
			if (prev_scene == SCENE_INTRO) snd_music(0);
			prev_scene = scene;
		}
		if (scene == SCENE_INTRO) snd_music(1);
		if (g_quit_req) running = 0;
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				running = 0;
			else if (ev.type == SDL_KEYDOWN) {
				SDL_Keycode k = ev.key.keysym.sym;
				int changed = 1;
				if (k == SDLK_ESCAPE)
					running = 0;
				else if ((scene == SCENE_ROOM || (scene == SCENE_WALK && !game_mode)) && k == SDLK_PAGEDOWN) { g_room.room++; g_room.scroll_x = g_room.scroll_y = 0; }
				else if ((scene == SCENE_ROOM || (scene == SCENE_WALK && !game_mode)) && k == SDLK_PAGEUP) { g_room.room--; g_room.scroll_x = g_room.scroll_y = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_RIGHTBRACKET) { g_room.level = g_room.level % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_LEFTBRACKET) { g_room.level = (g_room.level + (g_room.use_tata ? 2 : 3)) % (g_room.use_tata ? 4 : 5) + 1; g_room.room = 0; }
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_WALK || scene == SCENE_OBJECTS) && k == SDLK_t && g.id == GAME_PAYOFF) { g_room.use_tata ^= 1; g_room.level = 1; g_room.room = 0; }
				else if (scene == SCENE_ROOM && k == SDLK_LEFT) g_room.scroll_x -= 16;
				else if (scene == SCENE_ROOM && k == SDLK_RIGHT) g_room.scroll_x += 16;
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_OBJECTS) && k == SDLK_UP) g_room.scroll_y -= 16;
				else if ((scene == SCENE_ROOM || scene == SCENE_TILES || scene == SCENE_OBJECTS) && k == SDLK_DOWN) g_room.scroll_y += 16;
				else if (game_mode && (k == SDLK_SPACE || k == SDLK_RETURN || (k == SDLK_p && g.id == GAME_PAYOFF))) {
					/* the launcher of Cadaver 2 asks "Cadaver or The Payoff": Space = Cadaver (DATA), P = The Payoff (TATA) */
					if (scene == SCENE_INTRO) { scene = SCENE_WALK; g_room.level = 1; g_room.room = 0; g_room.use_tata = (k == SDLK_p) || start_tata; }
					else changed = 0;
				}
				else if (scene == SCENE_WALK && k == SDLK_PAUSE) { g_paused ^= 1; snd_silence(); if (!g_paused) { g_room.snd_key = 0; g_room.nroom_snd = 0; g_room.push_snd = 0; g_room.pending_rebuild = 1; } changed = 0; }
				else if (scene == SCENE_WALK && g_savemode) { savemode_key(k); changed = 0; }
				else if (scene == SCENE_WALK && (k == SDLK_s || k == SDLK_l) && g_room.ok && g_room.dead_timer == 0) { savemode_start(k == SDLK_s); changed = 0; }
				else if (scene == SCENE_WALK && k == SDLK_c) { for (int i = 0; i < 4; i++) g_room.msgs[i].active = 0; changed = 0; }   /* C: clear all game messages */
				else if (scene == SCENE_WALK && k == SDLK_F4) { g_four_dir ^= 1; changed = 0; }
				else if (k == SDLK_F11) { SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP); changed = 0; }
				else if (scene == SCENE_WALK && (k == SDLK_F5 || k == SDLK_F6) && g_room.ok && g_room.dead_timer == 0) { savemode_start(k == SDLK_F5); changed = 0; }   /* the original's keys (1000:0247: scancodes 3Fh / 40h -> 10F68: save / load) */
				else if (scene == SCENE_WALK && k == SDLK_F7) { g_paused ^= 1; snd_silence(); if (!g_paused) { g_room.snd_key = 0; g_room.nroom_snd = 0; g_room.push_snd = 0; g_room.pending_rebuild = 1; } changed = 0; }   /* 41h: pause */
				else if (scene == SCENE_WALK && k == SDLK_F8) { for (int i = 0; i < 4; i++) g_room.msgs[i].active = 0; changed = 0; }   /* 42h: clear the messages (13514) */
				else if (scene == SCENE_WALK && k == SDLK_F10) { game_save(&g_walk, &g_room, -1); changed = 0; }   /* a quick save without the price (not in the original) */
				else if (scene == SCENE_WALK && k == SDLK_F9) { game_load(&g_room, -1); changed = 0; }
				else if (scene == SCENE_WALK && k == SDLK_F1) { map_open_view(&g_room); changed = 0; }
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
				g_intro_t += 0.020;
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
			           (ks[SDL_SCANCODE_RIGHT] ? 8 : 0) | (ks[SDL_SCANCODE_INSERT] ? 5 : 0) | (ks[SDL_SCANCODE_DELETE] ? 6 : 0) | (ks[SDL_SCANCODE_PAGEUP] ? 9 : 0) | (ks[SDL_SCANCODE_PAGEDOWN] ? 10 : 0) | (ks[SDL_SCANCODE_END] ? 0x80 : 0) | (ks[SDL_SCANCODE_SPACE] ? 0x10 : 0) | (ks[SDL_SCANCODE_RETURN] ? 0x20 : 0) | (ks[SDL_SCANCODE_MINUS] ? 0x40 : 0) | (ks[SDL_SCANCODE_H] ? 0x100 : 0);
			if (g_paused) last_ms = now;
			while (now - last_ms >= 20) {            /* the game runs at ~50 Hz */
				walker_tick(&g_walk, &g_room, keys);
				last_ms += 20;
			}
			if (sim && frames == 0) {                 /* headless: --sim right:30,down:10 */
				for (const char *c = sim; *c;) {
					int bits = !strncmp(c, "rend", 4) ? 0x88 : !strncmp(c, "rdown", 5) ? 0x0A : !strncmp(c, "ldown", 5) ? 0x06 : !strncmp(c, "rup", 3) ? 0x09 : !strncmp(c, "lup", 3) ? 0x05 : !strncmp(c, "right", 5) ? 8 : !strncmp(c, "left", 4) ? 4 : !strncmp(c, "down", 4) ? 2 : !strncmp(c, "up", 2) ? 1 : !strncmp(c, "end", 3) ? 0x80 : !strncmp(c, "space", 5) ? 0x10 : !strncmp(c, "ret", 3) ? 0x20 : !strncmp(c, "hold", 4) ? 0x100 : 0;
					const char *colon = strchr(c, ':');
					if (!colon) break;
					if (!strncmp(c, "info", 4)) g_room.show_status ^= 1;
					if (!strncmp(c, "magicmap", 8)) map_open_magic(&g_room);
					else if (!strncmp(c, "map", 3)) map_open_view(&g_room);
					if (!strncmp(c, "key:", 4) && c[4]) {                 /* a typed key for the S / L prompts: key:s key:y key:3 ... */
						SDL_Keycode kc = (SDL_Keycode)c[4];
						if (g_savemode) savemode_key(kc); else if (kc == SDLK_s || kc == SDLK_l) savemode_start(kc == SDLK_s);
					}
					if (!strncmp(c, "dump:", 5)) { fprintf(stderr, "DUMP %d %d %d %d\n", g_walk.x1, g_walk.y1, g_walk.z, g_room.room); if (getenv("CAD_DUMPBOX")) for (int bi = 0; bi < g_room.ro.n; bi++) { const RoomObj *bo = &g_room.ro.o[bi]; fprintf(stderr, "OBJ %d %d %d %d %d %d %d %d\n", bo->thing, bo->cx1, bo->cy1, bo->cx0, bo->cy0, bo->czt, bo->czb, bo->sp->header[22]); }   /* debugging aid: print the player position */ }
					if (!strncmp(c, "money:", 6)) g_room.vm.money = (uint32_t)atoi(c + 6);
					if (!strncmp(c, "tp:", 3)) { int tx = 0, ty = 0, tz = 0; if (sscanf(c + 3, "%d:%d:%d", &tx, &ty, &tz) >= 2) { g_walk.x1 = tx; g_walk.y1 = ty; g_walk.z = tz; } }   /* debugging aid: put the player at x, y (z) */
					if (!strncmp(c, "save", 4)) game_save(&g_walk, &g_room, -1);
					if (!strncmp(c, "load", 4)) { game_load(&g_room, -1); load_scene(&g, scene, &scr); }
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
		{
			int ww, wh;
			SDL_GetRendererOutputSize(ren, &ww, &wh);
			SDL_Rect dst = {0, 0, ww, wh};
			if (ww * 3 > wh * 4) { dst.w = wh * 4 / 3; dst.x = (ww - dst.w) / 2; }
			else { dst.h = ww * 3 / 4; dst.y = (wh - dst.h) / 2; }
			SDL_RenderCopy(ren, tex, NULL, &dst);
		}
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
	if (g_adev) SDL_CloseAudioDevice(g_adev);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
