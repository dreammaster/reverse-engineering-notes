/*
 * origsave.h - reading a save file written by the original game (1000:C057, docs/formats.md "The original's save file").
 *
 * Layout: 8 bytes "mjm0DAC" + level - 1; then the sections 3 (rooms), 4 (doors), 5 (per-room thing lists), 8 (carried list) and 6 (things): u16 length, the records back to back, an index of
 * {u16 record length, u16 offset} per record (100, 400, 100, n carried, 1000 entries); then the data-segment block ds:020C .. 06F7 (1260 bytes).  The records of sections 3, 4 and 6 are
 * the same records as the level files' (MP2 sections 0, 1 and 3) in their run-time state, so a loaded game is the level plus these records instead of the file's.
 */
#ifndef CADAVER_ORIGSAVE_H
#define CADAVER_ORIGSAVE_H

#include <stddef.h>
#include <stdint.h>

#define ORIGSAVE_ROOMS 100
#define ORIGSAVE_DOORS 400
#define ORIGSAVE_LISTS 100
#define ORIGSAVE_THINGS 1000
#define ORIGSAVE_BLOCK_BASE 0x20C
#define ORIGSAVE_BLOCK_LEN 1260

typedef struct {
	uint8_t *buf;                                   /* the whole file (owned) */
	size_t n;
	int level;                                      /* 1..5 */
	struct { const uint8_t *p; int len; } room[ORIGSAVE_ROOMS], door[ORIGSAVE_DOORS], thing[ORIGSAVE_THINGS];
	const uint8_t *block;                           /* ds:020C .. 06F7 */
} OrigSave;

/* takes ownership of `data` (malloc'ed) on success; returns 0, or a negative code */
int origsave_parse(OrigSave *o, uint8_t *data, size_t n);
void origsave_free(OrigSave *o);
/* byte / word / dword of the data-segment block at the data segment offset `ds` (0 when outside the block) */
unsigned origsave_b(const OrigSave *o, int ds);
unsigned origsave_w(const OrigSave *o, int ds);
uint32_t origsave_d(const OrigSave *o, int ds);

#endif
