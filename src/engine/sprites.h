/*
 * sprites.h - PCMAN.DAT player sprite set (docs/formats.md section 6.2).
 */
#ifndef CADAVER_SPRITES_H
#define CADAVER_SPRITES_H

#include <stddef.h>
#include <stdint.h>

#define SPRITE_TRANSPARENT 0xFF
#define PCMAN_SPRITE_COUNT 48

typedef struct {
	int hot_x, hot_y;  /* offsets stored in the file (copied into the object by the game) */
	int w, h;          /* pixels */
	uint8_t *pixels;   /* w*h colour indices 0..15, SPRITE_TRANSPARENT where the mask is clear */
} Sprite;

typedef struct {
	int count;
	Sprite sprites[PCMAN_SPRITE_COUNT];
} SpriteSet;

/* `data` = unpacked PCMAN.DAT. Returns 0 on success. */
int pcman_load(const uint8_t *data, size_t len, SpriteSet *set);
void spriteset_free(SpriteSet *set);

/* The game palette: 16 Atari-ST words (0x0RGB, 3 bits per channel), stored in
 * every LEVn.MP1 and in the executables. Convert to 8-bit RGB. */
extern const uint16_t game_palette_st[16];
void st_word_to_rgb(uint16_t w, uint8_t rgb[3]);

#endif
