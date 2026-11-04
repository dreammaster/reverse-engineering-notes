/*
 * screen.h - the 320x200, 16-colour (EGA mode 0Dh style) display model.
 *
 * The original draws into planar VGA memory; the port keeps a chunky
 * 1-byte-per-pixel framebuffer plus an RGB palette and lets the platform layer
 * scale it.
 */
#ifndef CADAVER_SCREEN_H
#define CADAVER_SCREEN_H

#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200

typedef struct {
	uint8_t pixels[SCREEN_W * SCREEN_H];
	uint8_t palette[256][3];
} Screen;

/* Convert to 0x00RRGGBB, `out` has SCREEN_W*SCREEN_H entries. */
static inline void screen_to_rgb32(const Screen *s, uint32_t *out) {
	for (int i = 0; i < SCREEN_W * SCREEN_H; i++) {
		const uint8_t *c = s->palette[s->pixels[i]];
		out[i] = ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | c[2];
	}
}

#endif
