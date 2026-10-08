/* The recompiled game: host layer shared by the game_*.c files.  See gen/README.md for the model: the original program's memory
 * (image, DGROUP, stack in DGROUP) is MEM[]; translated routines run on a Cpu; everything the original got from DOS, the C runtime and
 * the video / sound modules is a host_* function written by hand. */
#ifndef MM3_GAME_H
#define MM3_GAME_H

#include <stdint.h>

#include "cc.h"
#include "gfx.h"
#include "recomp.h"
#include "ui_text.h"

typedef struct {
	char data_dir[512];
	Mm3Cc cc;          /* MM3.CC: graphics, text, sounds (LZHUF members) */
	Mm3Cc cur;         /* MM3.CUR: the maze files and the saved state (stored members) */
	Mm3Palette palette;
	uint16_t scale_patterns[4];
	Mm3Ui *ui;         /* the text engine; its screen is video memory (A000h) */
} Game;

extern Game G;

int game_init(const char *data_dir);      /* load the image, open the archives, set up memory; returns 0 on success */
void game_call(void (*fn)(Cpu *c), Cpu *c, const uint16_t *args, int nargs); /* far call of a translated routine (cdecl) */

/* memory: paragraph allocator over the free areas of the address space (the DOS heap of the original) */
uint16_t dos_alloc(uint32_t bytes);       /* segment, 0 when out of memory */
void dos_free(uint16_t seg);
uint32_t dos_block_size(uint16_t seg);

/* resources */
uint16_t game_load_resource(const char *name, uint32_t *size); /* segment of a fresh copy of the member, 0 when missing */

/* video / input (game_video.c) */
void video_present(void);
void video_pump_events(void);
void game_sprite_free_cache(uint16_t seg);
void game_exec_draw_list(unsigned list_addr);
int sound_init(int headless);  /* game_sound.c */
void sound_pump(int headless_polls);

#define SCREEN_MEM (MEM + 0xA0000)

#endif
