/* Text-mode screens drawn on the 320x200 canvas (shared by the SDL explorer and the screenshot tool). */
#ifndef MM2_UI_H
#define MM2_UI_H

#include "mm2_data.h"
#include "mm2_inn.h"
#include "mm2_text.h"

void mm2_ui_draw_inn(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town);

#endif
