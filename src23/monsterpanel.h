#ifndef YENDOR23_MONSTERPANEL_H
#define YENDOR23_MONSTERPANEL_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "viewrender.h"

/*
 * The monster info panels on the right of the combat screen: DrawMonsterInfoPanels (yendor2.asm:33942) and DrawMonsterInfoPanel
 * (:34262), DrawMonsterHealthBar (:34169). Three panels at x = 241, y = 87 / 123 / 159, one per monster slot (empty slot: nothing).
 *   - the two name lines (monster record +0x32 and +0x3F), 6 pixels apart, font 0 transparent; colour 0xAA once the monster's
 *     info was revealed (state bit 0x20), else 0x8A for the active combat monster, else 9
 *   - from a party reveal tier of 55 (Chapter 3: 60) up: a 45 x 8 health bar under the names (colour 0x59, 2 brighter when above
 *     the maximum, background 6): width 4500 / (100 * max / max(health, 1)), at least 1
 *   - from tier 75: three 8 x 8 status icons (category 9) at x + 46, + 55, + 64: 7 / 6 / 4 for state 0x4000 / 0x8000 / none,
 *     9 / 8 / 4 for 0x1000 / 0x2000 / none, 13 / 12 / 4 for 0x400 / 0x800 / none (4 = blank)
 *   - from tier 80, 9 pixels further down, text lines chosen by which quality bit of the state is set (0x200, then 0x100, 0x80,
 *     0x40): 0x200 -> POISONED (0x8000) and DISEASED (0x4000); 0x100 -> PARALYZED (0x2000) and FROZEN (0x1000); 0x80 -> HEXED
 *     (0x800) and CURSED (0x400); 0x40 -> "HEALTH:" and a "current/maximum" line
 * The quality bits 0x3E0 of the state are cleared afterwards (the panel is a one-shot display of what a spell revealed).
 */
enum { MonsterPanelCount = 3, MonsterPanelX = 241, MonsterPanelBarWidth = 45, MonsterPanelBarHeight = 8 };

/* The pixel width of the health bar fill. */
unsigned monsterPanelBarWidth(unsigned health, unsigned maximum);

/* y of panel 0-2. */
int monsterPanelY(unsigned panel);

/* Draws one panel for a monster record (156 bytes; updated: state bits 0x3E0 cleared). */
void monsterPanelDraw(const ViewRenderer *r, unsigned panel, uint8_t *monster, bool active, unsigned revealTier);

#endif
