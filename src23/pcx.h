#ifndef YENDOR23_PCX_H
#define YENDOR23_PCX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"

/*
 * Chapter 3's boot logo is a ZSoft PCX file: 640 x 480, eight bits, run length coded, with a 256 colour palette in the last 768 bytes. The program (sub_25A4E, yendor3.asm:38632) reads
 * its 29214 bytes (0x721E) from WORLD.DAT at the dword at DS:0xB20B (0x41EAB7), writes them to a file named LOGO.PCX (DS:0xF9F4), and sub_258DB decodes that file straight into the
 * VESA 640 x 480 mode (sub_258A8, mode 0x101): the row width is the header's BytesPerLine (word at +0x42; 640), the run length data runs from byte 128 for `size - 0x380` bytes (the last byte
 * of that range is the 0x0C palette marker, written as a pixel past the picture), a byte below 0xC0 is a pixel and a byte from 0xC0 a run of (byte - 0xC0) of the next byte (the original
 * computes the count as byte + 0x40 in eight bits), and the palette is read from the last 768 bytes and shifted right by two to the VGA's six bits.
 */
enum { PcxHeaderSize = 128, PcxPaletteSize = 768, PcxLogoSize = 0x721E };

/* Decodes a PCX: pixels = BytesPerLine * (ymax + 1) bytes into `pixels` (capacity bytes at most), `palette` the six-bit DAC values. False for a bad header, a size that does not fit or a
 * missing palette marker. */
bool pcxDecode(const uint8_t *data, size_t size, uint8_t *pixels, size_t capacity, unsigned *width, unsigned *height, uint8_t palette[PcxPaletteSize]);

/* Where the logo is in WORLD.DAT (Chapter 3 only: the dword at DS:0xB20B of the executable); false for Chapter 2 or when the table is missing. */
bool pcxLogoLocation(const ExeData *exe, GameKind game, size_t *offset, size_t *size);

#endif
