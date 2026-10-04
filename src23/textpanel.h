#ifndef YENDOR23_TEXTPANEL_H
#define YENDOR23_TEXTPANEL_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd4.h"
#include "game.h"
#include "viewrender.h"

/*
 * The text panel on the right of the main screen, below the minimap and icon row (frame picture region 4/5): ClearStatusPanelIfDirty
 * (yendor2.asm:11486), ClearMessageBoxArea (:11641), DrawStringColumn (:35154), ShowMaterialCounterHud (:12557). Outside combat two bands
 * are cleared to colour 4: a header line (72 x 6 at (241, 87)) and the message area (72 x 60 at (240, 96)); in combat the whole 73 x 109
 * block at (240, 86) is one area (the monster panels). Messages are lines of 6 pixels at (240, 96), colour 0x8A (the header's colour),
 * transparent over the cleared panel.
 */
void textPanelClear(const ViewRenderer *r, bool combat);

/* DrawStringColumn: the lines one under the other starting at (240, 96), 6 pixels apart. */
void textPanelMessage(const ViewRenderer *r, const char *const *lines, unsigned count, uint8_t colour);

/* The header line: text at (241, 87) in 0x8A over colour 4 (opaque). */
void textPanelHeader(const ViewRenderer *r, const char *text);

/* ShowMaterialCounterHud: "$" and the party's gold (comma-grouped) in the header. */
void textPanelGold(const ViewRenderer *r, const Bcd4 gold);

#endif
