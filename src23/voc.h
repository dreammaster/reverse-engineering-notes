#ifndef YENDOR23_VOC_H
#define YENDOR23_VOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Creative Voice Files, the format of the sound effects in WORLD.DAT (audio.h): "Creative Voice File" 0x1A, u16 header size (0x1A),
 * u16 version (0x010A), u16 check (0x1129), then blocks {u8 type, u24 length, data} ending with type 0. Every effect of both games is
 * a single type-1 block (u8 frequency divisor, u8 codec = 0, then unsigned 8-bit mono PCM) followed by the terminator; the sample
 * rate is 1,000,000 / (256 - divisor) (4,000-13,000 Hz in practice).
 */
typedef struct {
    unsigned sampleRate;
    const uint8_t *samples; /* unsigned 8-bit PCM */
    size_t length;
} VocSound;

/* Parses a single-sound voice file; false for anything else (other block types, codecs, truncation). */
bool vocOpen(VocSound *sound, const uint8_t *data, size_t size);

#endif
