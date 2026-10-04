#ifndef YENDOR23_EXEDATA_H
#define YENDOR23_EXEDATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The executables carry the games' fixed text and small tables in their 16-bit data segment (DS), and neither is packed or wrapped:
 * Chapter 2's SW.EXE and Chapter 3's REGISTER.EXE (the playable Chapter 3 program; the 4.6 MB Yendor3-full.exe next to it is a different,
 * packed wrapper) hold DS verbatim in the file. The IDA address "DS:0xNNNN" of anything in the disassembly is therefore at file offset
 * exeDataFileOffset(game) + 0xNNNN:
 *   Chapter 2 (SW.EXE, 0x21660 + DS offset)       Chapter 3 (REGISTER.EXE, 0x21DB0 + DS offset)
 * exeDataOpen checks that offset by looking for the label `QUIT "CREATE"` at its known place (DS:0x79E5 / 0x7D17).
 */
typedef struct {
    const uint8_t *data; /* the whole executable */
    size_t size;
    size_t base;         /* file offset of DS:0 */
} ExeData;

size_t exeDataFileOffset(GameKind game);

/* False if the file is too short or the check string is not where the data segment says it is. */
bool exeDataOpen(ExeData *exe, GameKind game, const uint8_t *data, size_t size);

/* The NUL-terminated string at DS:offset, truncated to `capacity - 1` characters; false if the offset is outside the file. */
bool exeDataString(const ExeData *exe, unsigned dsOffset, char *out, size_t capacity);

/* Byte / word at DS:offset (little endian); 0 outside the file. */
unsigned exeDataU8(const ExeData *exe, unsigned dsOffset);
unsigned exeDataU16(const ExeData *exe, unsigned dsOffset);

#endif
