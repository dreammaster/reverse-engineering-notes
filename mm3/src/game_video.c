/* The video module (vdrv_*), keyboard and SDL window of the recompiled game.  The original's video module is a separate piece of x86
 * code; here the API it exported is implemented on top of ui_text.c (windows and text), gfx.c (sprites) and SDL. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint32_t pixels[MM3_SCREEN_W * MM3_SCREEN_H];
static int headless;

/* ---- sprite sets: resources loaded into machine memory, decoded on first use */
typedef struct { uint16_t seg; Mm3Sprite spr; int valid; } CachedSprite;
static CachedSprite cache[256];

static Mm3Sprite *sprite_for_seg(uint16_t seg) {
	for (int i = 0; i < 256; i++)
		if (cache[i].valid && cache[i].seg == seg) return &cache[i].spr;
	for (int i = 0; i < 256; i++)
		if (!cache[i].valid) {
			uint32_t size = dos_block_size(seg);
			if (!size || mm3_sprite_decode(&cache[i].spr, SEGP(seg), size)) return NULL;
			cache[i].seg = seg;
			cache[i].valid = 1;
			return &cache[i].spr;
		}
	return NULL;
}

void game_sprite_free_cache(uint16_t seg) {
	for (int i = 0; i < 256; i++)
		if (cache[i].valid && cache[i].seg == seg) {
			mm3_sprite_free(&cache[i].spr);
			cache[i].valid = 0;
		}
}

static void draw_frame(uint16_t seg, unsigned frame, int x, int y, unsigned flags) {
	Mm3Sprite *s = sprite_for_seg(seg);
	if (s && frame < s->count)
		mm3_blit_ex(SCREEN_MEM, MM3_SCREEN_W, MM3_SCREEN_H, &s->frames[frame], x, y, flags, G.scale_patterns);
}

/* a draw list (docs/view.md): FFFF, offset, segment selects the sprite set (segment 0 ends the list); else x, y, flags, frame */
void game_exec_draw_list(unsigned addr) {
	uint16_t seg = 0;
	for (int n = 0; n < 4000; n++) {
		unsigned w = rd16(DG, (uint16_t)addr);
		if (w == 0xFFFF) {
			seg = rd16(DG, (uint16_t)(addr + 4));
			addr += 6;
			if (!seg) break;
		} else {
			draw_frame(seg, rd16(DG, (uint16_t)(addr + 6)), (int16_t)rd16(DG, (uint16_t)addr), (int16_t)rd16(DG, (uint16_t)(addr + 2)), rd16(DG, (uint16_t)(addr + 4)));
			addr += 8;
		}
	}
}

static void ui_draw_list(void *user, unsigned addr) { (void)user; game_exec_draw_list(addr); }

/* ---- SDL */
static int init_sdl(void) {
	if (getenv("MM3_HEADLESS")) { headless = 1; return 0; }
	if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return -1; }
	window = SDL_CreateWindow("Might and Magic III", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MM3_SCREEN_W * 3, MM3_SCREEN_H * 3, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_RenderSetLogicalSize(renderer, MM3_SCREEN_W, MM3_SCREEN_H);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MM3_SCREEN_W, MM3_SCREEN_H);
	return window && renderer && texture ? 0 : -1;
}

void video_present(void) {
	for (int i = 0; i < MM3_SCREEN_W * MM3_SCREEN_H; i++) {
		const uint8_t *c = G.palette.rgb[SCREEN_MEM[i]];
		pixels[i] = 0xFF000000u | (c[0] << 16) | (c[1] << 8) | c[2];
	}
	if (headless) return;
	SDL_UpdateTexture(texture, NULL, pixels, MM3_SCREEN_W * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void video_save_bmp(const char *path) {
	SDL_Surface *s;
	video_present();
	s = SDL_CreateRGBSurfaceFrom(pixels, MM3_SCREEN_W, MM3_SCREEN_H, 32, MM3_SCREEN_W * 4, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
	SDL_SaveBMP(s, path);
	SDL_FreeSurface(s);
}

/* ---- keyboard: BIOS key codes (scan code << 8 | ascii) for _bioskey */
#define KEYQ 64
static unsigned keyq[KEYQ];
static int kq_head, kq_tail;
static unsigned script_keys[256];
static int script_n, script_pos;

static void push_key(unsigned code) { int n = (kq_tail + 1) % KEYQ; if (n != kq_head) { keyq[kq_tail] = code; kq_tail = n; } }

void video_set_key_script(const unsigned *keys, int n) { memcpy(script_keys, keys, n * sizeof(unsigned)); script_n = n; script_pos = 0; }

static unsigned bios_code(SDL_Keycode k, Uint16 mod) {
	static const unsigned letters[26] = { 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C };
	if (k >= SDLK_a && k <= SDLK_z) {
		unsigned ch = (unsigned)(k - SDLK_a + ((mod & KMOD_SHIFT) ? 'A' : 'a'));
		return (letters[k - SDLK_a] << 8) | ch;
	}
	if (k >= SDLK_0 && k <= SDLK_9) return (((k == SDLK_0) ? 0x0B : 0x02 + (unsigned)(k - SDLK_1)) << 8) | (unsigned)k;
	switch (k) {
	case SDLK_UP: return 0x4800; case SDLK_DOWN: return 0x5000; case SDLK_LEFT: return 0x4B00; case SDLK_RIGHT: return 0x4D00;
	case SDLK_HOME: return 0x4700; case SDLK_END: return 0x4F00; case SDLK_PAGEUP: return 0x4900; case SDLK_PAGEDOWN: return 0x5100;
	case SDLK_INSERT: return 0x5200; case SDLK_DELETE: return 0x5300;
	case SDLK_ESCAPE: return 0x011B; case SDLK_RETURN: return 0x1C0D; case SDLK_SPACE: return 0x3920; case SDLK_BACKSPACE: return 0x0E08; case SDLK_TAB: return 0x0F09;
	case SDLK_PERIOD: return 0x342E; case SDLK_COMMA: return 0x332C; case SDLK_MINUS: return 0x0C2D; case SDLK_SLASH: return 0x352F;
	default: break;
	}
	if (k >= SDLK_F1 && k <= SDLK_F10) return (unsigned)(0x3B + (k - SDLK_F1)) << 8;
	return 0;
}

void video_pump_events(void) {
	SDL_Event e;
	if (headless) {
		if (script_pos < script_n && kq_head == kq_tail) push_key(script_keys[script_pos++]);
		return;
	}
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) exit(0);
		if (e.type == SDL_KEYDOWN) {
			unsigned code = bios_code(e.key.keysym.sym, e.key.keysym.mod);
			if (code) push_key(code);
		}
	}
}

static const char *shot_path;
void video_set_shot(const char *path) { shot_path = path; }

static void headless_finish(void) {
	if (shot_path) video_save_bmp(shot_path);
	fprintf(stderr, "headless run finished (the game is waiting for input)\n");
	exit(0);
}

void host__bioskey(Cpu *c) {
	unsigned cmd = host_arg(c, 0);
	static int idle;
	if (headless && cmd == 1 && kq_head == kq_tail && script_pos >= script_n && ++idle > 3000) headless_finish();
	if (kq_head != kq_tail) idle = 0;
	for (;;) {
		video_pump_events();
		if (cmd == 1 || kq_head != kq_tail) break;
		video_present();
		if (headless) headless_finish();
		SDL_Delay(5);
	}
	if (kq_head == kq_tail) { c->ax = 0; return; }
	c->ax = (uint16_t)keyq[kq_head];
	if (cmd == 0) kq_head = (kq_head + 1) % KEYQ;
}

/* ---- the video module's API */
static const char *far_str(Cpu *c, int n) { return (const char *)(SEGP(host_arg(c, n + 1)) + host_arg(c, n)); }

void host_vdrv_06_closeWindows(Cpu *c) { mm3_ui_close_windows(G.ui, (int16_t)host_arg(c, 0)); }
void host_vdrv_1E_openWindow(Cpu *c) {
	uint16_t toff = host_arg(c, 8), tseg = host_arg(c, 9);
	mm3_ui_open_window(G.ui, host_arg(c, 0), host_arg(c, 1), host_arg(c, 2), host_arg(c, 3), host_arg(c, 4),
		(toff | tseg) ? (const char *)(SEGP(tseg) + toff) : NULL);
}
void host_vdrv_2D_printText(Cpu *c) { mm3_ui_print(G.ui, far_str(c, 0)); }

void host_vdrv_21_loadSprites(Cpu *c) { /* name far -> far pointer of the loaded resource in dx:ax */
	uint16_t seg = game_load_resource(far_str(c, 0), NULL);
	if (!seg) exit(4);
	c->dx = seg; c->ax = 0;
}
void host_vdrv_24_freeSprites(Cpu *c) { uint16_t seg = host_arg(c, 1); game_sprite_free_cache(seg); dos_free(seg); }
void host_vdrv_15_drawSprite(Cpu *c) { draw_frame(host_arg(c, 1), host_arg(c, 2), (int16_t)host_arg(c, 3), (int16_t)host_arg(c, 4), host_arg(c, 5)); }

void host_vdrv_0C_showRaw(Cpu *c) { /* a 320x200 picture by name (near pointer) */
	uint32_t size;
	uint16_t seg = game_load_resource((const char *)(DG + host_arg(c, 0)), &size);
	if (!seg) exit(4);
	memcpy(SCREEN_MEM, SEGP(seg), size < 64000 ? size : 64000);
	dos_free(seg);
}
void host_vdrv_0F_fade(Cpu *c) { (void)c; video_present(); }
void host_vdrv_03_hideMouse(Cpu *c) { (void)c; }
void host_vdrv_27_setCursor(Cpu *c) { (void)c; }
void host_vdrv_18_getMouse(Cpu *c) { (void)c; c->ax = 0; }
void host_vdrv_1B_animateCursor(Cpu *c) { (void)c; video_pump_events(); video_present(); if (!headless) SDL_Delay(2); }

int video_init(int headless_mode) {
	if (headless_mode) headless = 1;
	if (init_sdl()) return -1;
	G.ui = mm3_ui_create(&G.cc);
	if (!G.ui) return -1;
	mm3_ui_set_screen(G.ui, SCREEN_MEM);
	G.ui->draw_list = ui_draw_list;
	return 0;
}
