/* Loading of the MM2 data files (docs/file-formats.md).  Port of tools/mm2_data.py. */
#ifndef MM2_FILES_H
#define MM2_FILES_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint8_t *data;
	size_t size;
} Mm2Blob;

typedef struct {
	char dir[512];
} Mm2Game;

/* dir == NULL: use $MM2_DIR or the default GOG install path. */
void mm2_game_init(Mm2Game *g, const char *dir);

/* Raw file contents; blob.data == NULL on failure.  Free with mm2_blob_free. */
Mm2Blob mm2_read_file(const Mm2Game *g, const char *name);
void mm2_blob_free(Mm2Blob *b);

/* LZW container: u32 decompressed size followed by the LZW stream (in[0..n)). */
Mm2Blob mm2_lzw_blob(const uint8_t *in, size_t n);

Mm2Blob mm2_load_lzw_file(const Mm2Game *g, const char *name);

/* MAP.DAT: 60 chunks of 512 bytes (256 wall bytes then 256 flag bytes). */
#define MM2_MAPS 60
int mm2_load_map(const Mm2Game *g, int map, uint8_t out[512]);

/* ATTRIB.DAT: 64 bytes per map (docs/file-formats.md). */
int mm2_load_attrib(const Mm2Game *g, int map, uint8_t out[64]);

/* EVENTSI/O.DAT chunk of a map (indoor file for maps 0-4 and 17-59, outdoor 5-16).  Returns a blob
 * (data == NULL if the map has no events). */
Mm2Blob mm2_load_events(const Mm2Game *g, int map);

typedef struct {
	int nTriggers;
	const uint8_t *triggers;   /* 3 bytes each: cell (y<<4|x), script, facing mask */
	const uint8_t *scripts;    /* script area */
	size_t scriptsLen;
	const uint8_t *messages;   /* FFh terminated strings */
	size_t messagesLen;
} Mm2EventChunk;

int mm2_parse_events(const Mm2Blob *chunk, Mm2EventChunk *out);

#endif
