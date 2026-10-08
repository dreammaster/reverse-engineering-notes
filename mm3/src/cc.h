/* MM3 .CC archive reader (docs/mm3-re.md sections 2, 3): TOC cipher, LZHUF members, filename hash.
 * C port of tools/mm3_cc.py; the Python tool is the reference. */
#ifndef MM3_CC_H
#define MM3_CC_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint16_t id;     /* filename hash */
	uint32_t offset; /* payload offset in the file */
	uint16_t size;   /* payload size */
} Mm3CcEntry;

typedef struct {
	uint8_t *data;
	size_t len;
	Mm3CcEntry *entries;
	unsigned count;
} Mm3Cc;

/* Filename -> 16-bit id (case-insensitive). */
uint16_t mm3_name_id(const char *name);

/* LZHUF-decode a member payload (fill, fill, u16 BE size, stream).  Returns a malloc'd buffer, or NULL when the
 * payload is not an LZHUF stream (members can be stored). */
uint8_t *mm3_lzhuf_decode(const uint8_t *blob, size_t len, size_t *out_len);

/* Open from memory (takes a copy) or from a file.  Return 0 on success. */
int mm3_cc_open_mem(Mm3Cc *cc, const uint8_t *data, size_t len);
int mm3_cc_open(Mm3Cc *cc, const char *path);
void mm3_cc_close(Mm3Cc *cc);

/* Index of the member called `name`, or -1. */
int mm3_cc_find(const Mm3Cc *cc, const char *name);

/* Member contents (decompressed when LZHUF, else as stored).  malloc'd; NULL when missing. */
uint8_t *mm3_cc_read(const Mm3Cc *cc, const char *name, size_t *out_len);
uint8_t *mm3_cc_read_index(const Mm3Cc *cc, int index, size_t *out_len);

#endif
