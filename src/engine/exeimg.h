/*
 * exeimg.h - the memory image of a self-extracting DOS executable (PKLITE, Pro-Pack): the MZ file is loaded and its start-up code run on the 8086 interpreter
 * (engine/x86.c) until `marker` shows up in memory, i.e. until the packer's stub has written the original program. Used to read the plain-text story of
 * INTRO.EXE at run time without keeping a copy of it (docs/formats.md section 4).
 */
#ifndef CADAVER_EXEIMG_H
#define CADAVER_EXEIMG_H

#include <stddef.h>
#include <stdint.h>

#define EXEIMG_MEM 0x100000

/* `exe` = the packed file. On success returns 0 and `*mem` = a malloc'ed 1 MB image (the program was loaded at segment 1010h) holding `marker`; the caller frees it. */
int exeimg_unpack(const uint8_t *exe, size_t len, const char *marker, uint8_t **mem);

#endif
