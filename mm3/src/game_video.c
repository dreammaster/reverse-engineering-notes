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
	if (getenv("MM3_DUMPLIST")) fprintf(stderr, "draw seg %04X frame %u at %d,%d fl %X -> %s count %d\n", seg, frame, x, y, flags, s ? "ok" : "NOSPR", s ? (int)s->count : -1);
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
			draw_frame(seg, rd16(DG, (uint16_t)(addr + 6)), (int16_t)rd16(DG, (uint16_t)addr) + G.ui->draw_ox, (int16_t)rd16(DG, (uint16_t)(addr + 2)) + G.ui->draw_oy, rd16(DG, (uint16_t)(addr + 4)));
			addr += 8;
		}
	}
}

static void ui_draw_list(void *user, unsigned addr) { (void)user; game_exec_draw_list(addr); }

/* ---- SDL */
static int init_sdl(void) {
	if (getenv("MM3_HEADLESS")) { headless = 1; return 0; }
	if (SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return -1; }
	window = SDL_CreateWindow("Might and Magic III", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MM3_SCREEN_W * 3, (getenv("MM3_SQUARE") ? MM3_SCREEN_H : 240) * 3, SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_RenderSetLogicalSize(renderer, MM3_SCREEN_W, getenv("MM3_SQUARE") ? MM3_SCREEN_H : 240); /* 320x200 on a 4:3 monitor: pixels are 1.2 times as tall as wide (MM3_SQUARE=1: square pixels) */
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MM3_SCREEN_W, MM3_SCREEN_H);
	return window && renderer && texture ? 0 : -1;
}

/* mouse: position in screen pixels, buttons (1 left, 2 right); the cursor sprite is drawn on the presented picture only */
static int mouse_x = 160, mouse_y = 100, mouse_btn, mouse_seen, cursor_on, click_reads;
static uint16_t cursor_seg, cursor_frame;
static int fade_level = 16; /* 0..16 brightness */

void video_present(void) {
	static uint8_t shown[MM3_SCREEN_W * MM3_SCREEN_H];
	memcpy(shown, SCREEN_MEM, sizeof shown);
	if (cursor_on && mouse_seen) {
		Mm3Sprite *s = sprite_for_seg(cursor_seg);
		if (s && cursor_frame < s->count) mm3_blit_ex(shown, MM3_SCREEN_W, MM3_SCREEN_H, &s->frames[cursor_frame], mouse_x, mouse_y, 0, G.scale_patterns);
	}
	for (int i = 0; i < MM3_SCREEN_W * MM3_SCREEN_H; i++) {
		const uint8_t *c = G.palette.rgb[shown[i]];
		pixels[i] = 0xFF000000u | ((c[0] * fade_level / 16) << 16) | ((c[1] * fade_level / 16) << 8) | (c[2] * fade_level / 16);
	}
	if (headless) return;
	SDL_UpdateTexture(texture, NULL, pixels, MM3_SCREEN_W * 4);
	SDL_RenderClear(renderer);
	SDL_Rect whole = { 0, 0, MM3_SCREEN_W, getenv("MM3_SQUARE") ? MM3_SCREEN_H : 240 };
	SDL_RenderCopy(renderer, texture, NULL, &whole);
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
static int script_n, script_pos, script_wait;

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
		if (script_wait > 0) { script_wait--; return; }
		if (script_pos < script_n && (script_keys[script_pos] & 0x40000000u) && !(script_keys[script_pos] & 0x80000000u)) { script_wait = script_keys[script_pos++] & 0xFFFFF; return; } /* wN: let the game run N polls */
		if (script_pos < script_n && kq_head == kq_tail && !(script_keys[script_pos] & 0x80000000u)) push_key(script_keys[script_pos++]);
		return;
	}
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) exit(0);
		if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			int px, py;
			Uint32 b = SDL_GetMouseState(&px, &py);
			{ float lx, ly; SDL_RenderWindowToLogical(renderer, px, py, &lx, &ly); mouse_x = (int)lx; mouse_y = (int)(ly * MM3_SCREEN_H / (getenv("MM3_SQUARE") ? MM3_SCREEN_H : 240)); (void)w; (void)h; }
			mouse_btn = (b & SDL_BUTTON_LMASK ? 1 : 0) | (b & SDL_BUTTON_RMASK ? 2 : 0);
			mouse_seen = 1;
		}
		if (e.type == SDL_KEYDOWN && ((e.key.keysym.sym == SDLK_RETURN && (e.key.keysym.mod & KMOD_ALT)) || e.key.keysym.sym == SDLK_F11)) { /* Alt+Enter / F11: full screen */
			SDL_SetWindowFullscreen(window, (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
			continue;
		}
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
	static int idle, polls, idle_limit, every = -1;
	if (every < 0) { const char *e = getenv("MM3_SHOT_EVERY"), *l = getenv("MM3_IDLE"); every = e ? atoi(e) : 0; idle_limit = l ? atoi(l) : 3000; }
	if (cmd == 1 && every && ++polls % every == 0) { char fn[256]; snprintf(fn, sizeof fn, "%s_%04d.bmp", getenv("MM3_SHOT_PREFIX") ? getenv("MM3_SHOT_PREFIX") : "build/poll", polls / every); video_save_bmp(fn); }
	if (cmd == 1) sound_pump(headless);
	if (headless && cmd == 1 && kq_head == kq_tail && script_pos >= script_n && ++idle > idle_limit) headless_finish();
	if (kq_head != kq_tail) idle = 0;
	for (;;) {
		video_pump_events();
		if (cmd != 0 || kq_head != kq_tail) break;
		video_present();
		if (headless) headless_finish();
		SDL_Delay(5);
	}
	if (!headless && cmd == 1) { /* polling loop of the game: keep the picture current and the CPU cool */
		static Uint32 last_present;
		Uint32 now = SDL_GetTicks();
		if (now - last_present >= 16) { video_present(); last_present = now; }
		SDL_Delay(1);
	}
	if (cmd == 2) { /* BIOS shift status: 1 right shift, 2 left shift, 4 ctrl, 8 alt */
		SDL_Keymod m = headless ? 0 : SDL_GetModState();
		c->ax = (uint16_t)(((m & KMOD_RSHIFT) ? 1 : 0) | ((m & KMOD_LSHIFT) ? 2 : 0) | ((m & KMOD_CTRL) ? 4 : 0) | ((m & KMOD_ALT) ? 8 : 0));
		return;
	}
	if (kq_head == kq_tail) { c->ax = 0; return; }
	c->ax = (uint16_t)keyq[kq_head];
	if (cmd == 0 && getenv("MM3_TRACE")) fprintf(stderr, "key %04X\n", c->ax);
	if (cmd == 0) kq_head = (kq_head + 1) % KEYQ;
}

/* ---- the video module's API */
static const char *far_str(Cpu *c, int n) { return (const char *)(SEGP(host_arg(c, n + 1)) + host_arg(c, n)); }

void host_vdrv_06_closeWindows(Cpu *c) { mm3_ui_close_windows(G.ui, (int16_t)host_arg(c, 0)); }
void host_vdrv_1E_openWindow(Cpu *c) {
	uint16_t toff = host_arg(c, 8), tseg = host_arg(c, 9);
	if (getenv("MM3_TEXTLOG") && (toff | tseg)) { const char *t = (const char *)(SEGP(tseg) + toff); fprintf(stderr, "WINDOW %d,%d %dx%d:", host_arg(c, 0), host_arg(c, 1), host_arg(c, 2), host_arg(c, 3)); for (; *t; t++) { if ((unsigned char)*t < 32) fprintf(stderr, "<%02X>", (unsigned char)*t); else fputc(*t, stderr); } fputc('\n', stderr); }
	mm3_ui_open_window(G.ui, host_arg(c, 0), host_arg(c, 1), host_arg(c, 2), host_arg(c, 3), host_arg(c, 4),
		(toff | tseg) ? (const char *)(SEGP(tseg) + toff) : NULL);
}
void host_vdrv_2D_printText(Cpu *c) {
	if (getenv("MM3_TEXTLOG")) { const char *t = far_str(c, 0); fputs("TEXT:", stderr); for (; *t; t++) { if ((unsigned char)*t < 32) fprintf(stderr, "<%02X>", (unsigned char)*t); else fputc(*t, stderr); } fputc('\n', stderr); }
	mm3_ui_print(G.ui, far_str(c, 0));
}

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
/* vdrv_0F_fade(0 = out / 1 = in, speed): the palette fades to black and back; the picture underneath may change while it is black.
 * Headless runs skip the animation and stay at full brightness so screenshots always show the picture. */
void host_vdrv_0F_fade(Cpu *c) {
	int in = host_arg(c, 0) != 0;
	if (!headless) {
		for (int i = 0; i <= 16; i++) { fade_level = in ? i : 16 - i; video_present(); SDL_Delay(20); video_pump_events(); }
	}
	fade_level = in ? 16 : (headless ? 16 : 0);
	video_present();
}
void host_vdrv_03_hideMouse(Cpu *c) { (void)c; cursor_on = 0; }
void host_vdrv_27_setCursor(Cpu *c) { cursor_frame = host_arg(c, 0); cursor_seg = host_arg(c, 2); cursor_on = 1; }
/* The original's getMouse waits for the vertical retrace (70 Hz) before it returns: the game's delay loops (`getMouse` N times) rely on that
 * for their duration, so wait for the next 1/70 s tick here and show the picture. */
static void wait_retrace(void) {
	static Uint64 next;
	Uint64 now = SDL_GetPerformanceCounter(), freq = SDL_GetPerformanceFrequency(), period = freq / 70;
	if (now < next) SDL_Delay((Uint32)((next - now) * 1000 / freq));
	now = SDL_GetPerformanceCounter();
	next = (next && now - next < period * 4) ? next + period : now + period;
	video_present();
}

/* getMouse(&x, &y) -> buttons.  A scripted click ("mX:Y" in --keys) holds the button for a few polls, then releases */
void host_vdrv_18_getMouse(Cpu *c) {
	if (!headless) wait_retrace();
	video_pump_events();
	if (headless && !click_reads && script_wait <= 0 && script_pos < script_n && kq_head == kq_tail && (script_keys[script_pos] & 0x80000000u)) {
		unsigned v = script_keys[script_pos++];
		mouse_x = (v >> 12) & 0xFFF; mouse_y = v & 0xFFF; mouse_seen = 1; click_reads = 4;
	}
	if (headless) mouse_btn = click_reads > 0;
	if (click_reads > 0) click_reads--;
	if (!cursor_on) { c->ax = 0; wr16(SEGP(host_arg(c, 1)), host_arg(c, 0), 0); wr16(SEGP(host_arg(c, 3)), host_arg(c, 2), 0); return; }
	wr16(SEGP(host_arg(c, 1)), host_arg(c, 0), (uint16_t)mouse_x);
	wr16(SEGP(host_arg(c, 3)), host_arg(c, 2), (uint16_t)mouse_y);
	c->ax = (uint16_t)mouse_btn;
}
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

/* vdrv_2A_starfield: one frame of the intro's particle effect.  A 75-particle field bursts out of the screen centre
 * (positions/velocities are 16-bit fixed point, >>6 gives the pixel; velocity grows by 1/16 per frame, a particle is
 * respawned when it leaves 5000h x 3200h or its colour has faded to the target), then the draw list passed in is drawn on top.
 * Arguments: far pointer to the draw list.  The original seeds respawns from the DOS clock; here: a small LCG. */
#define NSTARS 75
static struct { int16_t x, vx, y, vy; uint8_t col, target; } stars[NSTARS];
static unsigned star_rng = 12345, star_parity;
static int star_rand(void) { star_rng = star_rng * 1103515245u + 12345u; return (int)(star_rng >> 16); }

void host_vdrv_2A_starfield(Cpu *c) {
	static const uint8_t cols[4] = { 0x1F, 0x1F, 0x9F, 0xDF };
	unsigned list = host_arg(c, 0);
	star_parity ^= 1;
	if (getenv("MM3_DUMPLIST")) { for (int k = 0; k < 20; k++) fprintf(stderr, "%04X ", rd16(DG, (uint16_t)(list + 2 * k))); fprintf(stderr, "\n"); }
	memset(SCREEN_MEM, 0, 64000);
	for (int i = 0; i < NSTARS; i++) {
		uint16_t x, y;
		stars[i].vx += stars[i].vx >> 4; stars[i].x += stars[i].vx;
		x = (uint16_t)stars[i].x;
		int respawn = x >= 0x5000;
		if (!respawn) {
			stars[i].vy += stars[i].vy >> 4; stars[i].y += stars[i].vy;
			y = (uint16_t)stars[i].y;
			respawn = y >= 0x3200 || stars[i].col == stars[i].target;
		}
		if (respawn) {
			uint8_t al = cols[star_rand() & 3];
			stars[i].col = al; stars[i].target = al - 0x1F;
			stars[i].vx = (int8_t)star_rand(); stars[i].x = 0x2800 + (uint8_t)star_rand() * 4;
			stars[i].vy = (int8_t)star_rand(); stars[i].y = 0x1900 + (uint8_t)star_rand() * 4;
			x = (uint16_t)stars[i].x; y = (uint16_t)stars[i].y;
		}
		SCREEN_MEM[((x >> 6) + (y >> 6) * 320) & 0xFFFF] = stars[i].col;
		if (stars[i].col != stars[i].target) stars[i].col -= star_parity;
	}
	game_exec_draw_list(list);
	video_present();
	if (!headless) SDL_Delay(25);
}
/* vdrv_00(draw list far pointer): the list is drawn into an off-screen copy of the screen which then replaces it -- here: draw it onto the screen */
void host_vdrv_00_transition(Cpu *c) { if (host_arg(c, 1) == DSEG) game_exec_draw_list(host_arg(c, 0)); video_present(); }
