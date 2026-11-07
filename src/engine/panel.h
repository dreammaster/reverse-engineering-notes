/*
 * panel.h - the frame around the room view: ANDATA.DAT block 1 (docs/formats.md 6.3, tools/panel.py).
 * Verified: a composite of floor + walls + objects + this frame equals 98.2% of the original's 320x200 screen (level 1 room 0;
 * the rest is the player, one animated object and the status text).
 */
#ifndef CADAVER_PANEL_H
#define CADAVER_PANEL_H

#include <stddef.h>
#include <stdint.h>

/* ANDATA.DAT (unpacked): u16be count, count x u16be sizes, blocks back to back. Returns 0 on success. */
int andata_block(const uint8_t *data, size_t len, int index, const uint8_t **block, size_t *blen);

/* Draw the frame wings over a 320x200 canvas of colour indices (TILE_TRANSPARENT = 0xFF counts as 0). `b1` = block 1.
 * Returns 0 on success, -1 if the block is too short. */
int panel_apply_wings(const uint8_t *b1, size_t len, uint8_t *canvas);

/* ---- status text (docs/formats.md 6.3a, tools/status.py) ---- */
#define STATUS_GLYPH_H 6
#define STATUS_ADVANCE 7
#define STATUS_NAME_MAX 15

/* Glyph number of a character (ANDATA block 4), -1 for a space (advance only). */
int status_glyph_index(char ch);
/* Draw `s` with its left edge at x onto a 320x200 colour-index canvas. `font` = ANDATA block 4 (24 bytes per glyph). */
void status_draw_string(uint8_t *canvas, int x, int y, const char *s, const uint8_t *font, size_t font_len);
/* Draw `s` centred in the box (bx, by, bw): x = bx + (bw - 7*len)/2. */
void status_draw_centred(uint8_t *canvas, int bx, int by, int bw, const char *s, const uint8_t *font, size_t font_len);
/* The 20-word room-name table in the trailer of an unpacked LEVn.MP1. Returns 0 on success. */
int status_name_table(const uint8_t *mp1, size_t len, uint16_t table[20]);
/* Build the room name for a section-0 room record (>= 27 bytes): [prefix id at +0x18] ' ' [table[rec[0x1A]]], cut to 15 chars.
 * `strings`/`count` = ENGn.LNG. Returns the length. */
size_t status_room_name(const uint8_t *rec, size_t rec_len, const uint16_t table[20], char *const *strings, int count,
                        char *out, size_t outsz);
/* "DAY n" (box cleared first) and the room name. */
void status_draw(uint8_t *canvas, const uint8_t *font, size_t font_len, const char *room_name, int day);

#endif
