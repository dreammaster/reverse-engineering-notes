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

/* ---- verb icons in the six slots while touching an object (docs/formats.md 6.7b, tools/verbs.py) ---- */
#define VERB_SLOTS 6
/* hdr = 32-byte sprite block header (file order), flags2 = rec[15] of the touched thing, ds703 = game flag word (bit 0x20: no "look" verb).
 * Fills out[] (at most 6) and returns the count (the last verb is always 6). */
int verbs_list(const uint8_t hdr[32], int flags2, int ds703, uint8_t out[VERB_SLOTS]);
/* Draw the n verb icons (the rest of the six slots get the empty icon 0). icons = ANDATA block 0 (17 icons of 112 bytes). */
void verbs_draw(uint8_t *canvas, const uint8_t *verbs, int n, const uint8_t *icons, size_t icons_len);

/* Energy bar (1000:D8C3 / D8DE): 22 x 6 box at (277,143) cleared, then filled with colour 4 for 22 * energy / max pixels. */
void energy_bar_draw(uint8_t *canvas, int energy, int energy_max);
/* The level's maximum energy: big-endian word at offset 60 of the unpacked LEVn.MP1 (also the initial energy). */
int level_energy_max(const uint8_t *mp1, size_t len);

/* The menu cursor: a 18 x 16 frame (outer colour 0xA, inner 0xE) around slot `slot` (1000:2C36). */
void verbs_highlight(uint8_t *canvas, int slot);

#endif
