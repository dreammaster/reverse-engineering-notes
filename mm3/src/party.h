/* Party state (MAZE.PTY, 918 bytes) and the roster (MAZE.CHR, 30 x 303 bytes).  Offsets: docs/data-files.md. */
#ifndef MM3_PARTY_H
#define MM3_PARTY_H

#include <stddef.h>
#include <stdint.h>

#include "character.h"

#define MM3_PTY_SIZE 0x396
#define MM3_ROSTER_SIZE 30
#define MM3_MAX_PARTY 8
#define MM3_FLAG_BYTES 32

typedef struct {
	uint8_t count;
	uint8_t member[MM3_MAX_PARTY]; /* roster indexes, 0xFF = empty */
	uint8_t facing, x, y, map;
	uint8_t sound_fx, music, option, last_inn_town;
	uint8_t levitate, wizard_eye, walk_on_water;
	uint16_t day, year, minutes, food;
	uint16_t light, fire, elec, cold, poison; /* party resistance / light counters (words) */
	uint32_t bank_gold, bank_gems, gold, gems;
	uint8_t event_bytes[53];          /* 341h: per-index state bytes (event value mode 23) */
	uint8_t flags[MM3_FLAG_BYTES];    /* 376h: game-flag bit array (event value mode 20) */
	uint8_t raw[MM3_PTY_SIZE];        /* the whole block, for writing back unparsed parts */
} Mm3Party;

int mm3_party_load(Mm3Party *p, const uint8_t *data, size_t len);
int mm3_party_flag(const Mm3Party *p, unsigned bit);

/* Roster: copies sizeof(Mm3Character) bytes per record. */
int mm3_roster_load(Mm3Character *out, unsigned max, const uint8_t *data, size_t len);

#endif
