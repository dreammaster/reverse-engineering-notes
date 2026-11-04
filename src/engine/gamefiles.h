/*
 * gamefiles.h - locating and loading the original game's data files.
 *
 * Handles the things the DOS games did themselves: case-insensitive
 * DOS-style paths ("data\\?\\lev?.mp1"), the '?' disk/level placeholder,
 * and telling Cadaver 1 from Cadaver 2 (The Payoff).
 */
#ifndef CADAVER_GAMEFILES_H
#define CADAVER_GAMEFILES_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
	GAME_UNKNOWN = 0,
	GAME_CADAVER,          /* Cadaver (1990): CAD.EXE, DATA/1..5            */
	GAME_PAYOFF            /* Cadaver: The Payoff (1991): GAME.EXE, DATA + TATA */
} GameId;

typedef struct {
	char root[1024];
	GameId id;
} GameFiles;

/* Detect the game in `dir`. Returns 0 on success. */
int gamefiles_open(GameFiles *g, const char *dir);

const char *gamefiles_name(GameId id);

/* Resolve a DOS-style relative path (either slash, any case, '?' replaced by
 * `disk` if disk >= 0) to a real host path. Returns 0 and fills `out` if the
 * file exists. */
int gamefiles_resolve(const GameFiles *g, const char *dospath, int disk, char *out, size_t outsz);

/* Read a whole file (malloc'ed). Returns 0 on success. */
int gamefiles_read(const GameFiles *g, const char *dospath, int disk, uint8_t **data, size_t *len);

/* Read + mjm-decompress. */
int gamefiles_read_mjm(const GameFiles *g, const char *dospath, int disk, uint8_t **data, size_t *len);

#endif
