#ifndef YENDOR23_STATSHEET_H
#define YENDOR23_STATSHEET_H

#include <stdint.h>

#include "game.h"
#include "item.h"
#include "viewrender.h"

/*
 * The numbers on the character stat sheet: DrawCharacterStatSheet (yendor2.asm:36860, yendor3.asm:36628), drawn over the full-screen
 * picture category 0 / id 3 (which carries the labels). Everything is font 0, transparent, 10 pixels apart:
 *   left column, x = 203: record +0x3C..+0x46 (the six attributes) at y = 60, 70, ..., 110; +0x4C, +0x4E, +0x50 at y = 130, 140, 150;
 *     HP +0x52 and MP +0x54 at x = 191, y = 170 and 180; experience (Bcd4 at +0x18, comma-grouped) at y = 190
 *   right column, x = 297: +0x58..+0x66 at y = 60..130, then +0x68, +0x6A, +0x6C, +0x6E (and, in Chapter 2, +0x70) at y = 140..180 --
 *     the last five (four in Chapter 3) are the role skills: shown in colour 0xCB (0x9B when above the maximum) for the character
 *     holding the matching party role (save header 0xA4, five words), else 0xF / 0x8A
 * A value is drawn in colour 0xF, or 0x8A when it exceeds its natural maximum (the same field + 0x40). A level-1 character also shows
 * a verdict on the average attribute at (254, 26), colour 0xDF: POOR (<= 49), AVERAGE (<= 52), GOOD (<= 55) or GREAT.
 */
void statSheetDraw(const ViewRenderer *r, const uint8_t *partyRecord, unsigned characterId, const uint16_t roles[5]);

/*
 * The whole sheet (DrawCharacterSheetPanel, yendor2.asm:37569): the full-screen picture (category 0 id 3) at (1, 1), the title
 * `title` (the original's message 0x7962, "CHARACTER CREATION" on the creation path) at (107, 6), the paper doll at (116, 60), the
 * face (category 7 picture [+0x12], transparent) at (116, 19), the name at (156, 26), the class name at (156, 38) and the level at
 * (256, 38), then statSheetDraw's numbers.
 */
void characterSheetDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint8_t *partyRecord, unsigned characterId, const uint16_t roles[5],
                        const char *title);

/*
 * The party-member detail screen (RunPartyMemberDetailScreen, yendor2.asm:16307, with DrawTrainingScreenStatSheet :16614 and
 * SelectAndDrawPartyStatusRow :16739; the same in Chapter 3 apart from having four role skills): the frame (category 1 picture 1, opaque) at
 * (15, 23) with the labels baked in; the class name at (20, 27), the level at (122, 27) and the experience at (160, 27); the six attributes
 * `[+0x3C..+0x46]` at x = 42, y = 37 + 10k; `[+0x4C..+0x50]` at x = 42, y = 99, 109, 119; the skills `[+0x58..+0x66]` at x = 116, y = 43 + 10k;
 * the role skills `[+0x68..]` at x = 203, y = 43 + 10k (colour 0xCB for the holder of the role). Values above their natural maximum are 0x8A.
 */
void detailSheetDraw(const ViewRenderer *r, const uint8_t *partyRecord, unsigned characterId, const uint16_t roles[5]);

#endif
