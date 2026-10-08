/* viewer DATADIR NAME [FRAME] [--shot out.bmp]
 * First SDL front end: shows a raw screen (*.RAW, 320x200) or a sprite frame from MM3.CC with the master palette.
 * NAME is a member name (CREATE.RAW) or a 4-digit hex id.  Left/Right change the frame, Esc quits.
  --text "string" draws a line of text with the game font at (10, 150) (try the alternate font with \x02).
 --window "text" opens a parchment window (x 40, y 30, 240 x 110) with the text, using the game's text engine
 * (control codes: \x03c centre, \x0a new line, \x0c05 colour ...; the shell needs $'...' quoting).
 * With --shot the picture is written as a BMP and the program exits (works with SDL_VIDEODRIVER=dummy). */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "gfx.h"
#include "ui_text.h"

static int find_member(const Mm3Cc *cc, const char *name) {
	if (strlen(name) == 4 && strspn(name, "0123456789abcdefABCDEF") == 4) {
		unsigned id = (unsigned)strtoul(name, NULL, 16);
		for (unsigned i = 0; i < cc->count; i++)
			if (cc->entries[i].id == id)
				return (int)i;
	}
	return mm3_cc_find(cc, name);
}

int main(int argc, char **argv) {
	char path[1024];
	const char *shot = NULL, *text = NULL, *window = NULL;
	Mm3Font font;
	int frame = 0, member, quit = 0;
	Mm3Cc cc;
	Mm3Palette pal;
	Mm3Sprite spr = {0};
	uint8_t *data, screen[MM3_RAW_SIZE];
	size_t len;
	int is_raw = 0;
	SDL_Window *win = NULL;
	SDL_Renderer *ren = NULL;
	SDL_Texture *tex = NULL;
	SDL_Surface *surf;

	if (argc < 3) { fprintf(stderr, "usage: %s DATADIR NAME [FRAME] [--shot out.bmp]\n", argv[0]); return 2; }
	for (int i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
		else if (!strcmp(argv[i], "--text") && i + 1 < argc) text = argv[++i];
		else if (!strcmp(argv[i], "--window") && i + 1 < argc) window = argv[++i];
		else frame = atoi(argv[i]);
	}
	snprintf(path, sizeof path, "%s/MM3.CC", argv[1]);
	if (mm3_cc_open(&cc, path) || mm3_palette_load(&pal, &cc)) { fprintf(stderr, "cannot load %s\n", path); return 1; }
	member = find_member(&cc, argv[2]);
	data = member >= 0 ? mm3_cc_read_index(&cc, member, &len) : NULL;
	if (!data) { fprintf(stderr, "no member %s\n", argv[2]); return 1; }
	if (len == MM3_RAW_SIZE) is_raw = 1;
	else if (mm3_sprite_decode(&spr, data, len)) { fprintf(stderr, "%s is not a graphics resource\n", argv[2]); return 1; }

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
		if (is_raw) memcpy(screen, data, MM3_RAW_SIZE);
		else {
			memset(screen, 0, sizeof screen);
			if (frame >= (int)spr.count) frame = (int)spr.count - 1;
			if (frame < 0) frame = 0;
			mm3_blit(screen, MM3_SCREEN_W, MM3_SCREEN_H, &spr.frames[frame], 10, 10);
		}
		if (window) {
			Mm3Ui *ui = mm3_ui_create(&cc);
			if (ui) {
				memcpy(ui->screen, screen, MM3_RAW_SIZE);
				mm3_ui_open_window(ui, 40, 30, 240, 110, 1, window);
				memcpy(screen, ui->screen, MM3_RAW_SIZE);
				mm3_ui_destroy(ui);
			}
		}
		if (text && mm3_font_load(&font, &cc) == 0) {
			static const uint8_t colors[3] = { 0x40, 0x30, 0x20 }; /* the module's initial colour table (B75h..B77h) */
			mm3_font_draw_text(&font, screen, MM3_SCREEN_W, MM3_SCREEN_H, 10, 150, text, colors);
		}
		for (int i = 0; i < MM3_RAW_SIZE; i++) {
			const uint8_t *c = pal.rgb[screen[i]];
			((uint32_t *)surf->pixels)[i] = 0xFF000000u | (c[0] << 16) | (c[1] << 8) | c[2];
		}
		if (shot) { SDL_SaveBMP(surf, shot); break; }
		SDL_UpdateTexture(tex, NULL, surf->pixels, surf->pitch);
		SDL_RenderClear(ren);
		SDL_RenderCopy(ren, tex, NULL, NULL);
		SDL_RenderPresent(ren);
		while (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) quit = 1;
			else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RIGHT) frame++;
			else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_LEFT) frame--;
			else continue;
			break;
		}
	}
	SDL_Quit();
	mm3_sprite_free(&spr);
	free(data);
	mm3_cc_close(&cc);
	return 0;
}
