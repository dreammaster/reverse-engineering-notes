#ifndef YENDOR23_CONFIRMPROMPT_H
#define YENDOR23_CONFIRMPROMPT_H

#include <stdbool.h>
#include <stdint.h>

#include "exedata.h"
#include "game.h"
#include "viewrender.h"

/*
 * ShowConfirmPrompt's messages (yendor2.asm:18567, Chapter 3 :10554): the executable's table of prompts (DS:0xA787 in Chapter 2, DS:0xE265 in Chapter 3; 2 bytes per message id, a pointer to the
 * message). A message is four words -- the mode flags, the number of text groups, the first and the last click region that answers it -- and then one ten-byte record per group: the colour, x,
 * y, a pointer to its first string and the number of lines (consecutive NUL-terminated strings, 6 pixels apart). The mode picks how it is answered:
 *   0x8000  yes / no: Y or Enter answer the first region, N or Escape the last; a click in the regions answers with that region's number (5 = YES row, 6 / 7 = NO in Chapter 2)
 *   0x4000  a party member: a click on a portrait or F1-F4 answers (the record id), Escape or the right button give up
 *   0x2000  a typed word (EditTextField at (240, 114), 12 characters, 0x0F on colour 4): the password prompts
 *   0x1000  a click on one of the rows only (the first / last region), Escape or the right button give up: MARK / RETURN
 * The texts are the same in both chapters except message 0x22 (Chapter 3 asks "DO YOU WANT / TO... / MARK / RETURN TO" with the answers at y = 114 and 126, regions 4 and 6).
 */
enum { ConfirmPromptMaxLines = 12, ConfirmPromptTextSize = 24 };

typedef struct {
    uint8_t colour;
    int x, y;
    char text[ConfirmPromptTextSize];
} ConfirmPromptLine;

typedef struct {
    uint16_t flags;
    unsigned firstRegion, lastRegion;
    unsigned lineCount;
    ConfirmPromptLine lines[ConfirmPromptMaxLines];
} ConfirmPrompt;

enum { ConfirmModeYesNo = 0x8000, ConfirmModeMember = 0x4000, ConfirmModeText = 0x2000, ConfirmModeRows = 0x1000 };

/* The table's own address for a game, and false if the id is not in it (or the executable is short). */
unsigned confirmPromptTableOffset(GameKind game);
bool confirmPromptLoad(const ExeData *exe, GameKind game, unsigned id, ConfirmPrompt *out);

/* Writes the lines (transparent) into the message area; the caller has cleared it. */
void confirmPromptDraw(const ViewRenderer *r, const ConfirmPrompt *prompt);

#endif
