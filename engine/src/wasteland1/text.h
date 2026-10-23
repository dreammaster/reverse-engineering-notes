#pragma once
// Text engine and keyboard layer of WL.EXE (character writers, line buffer / word wrap / scrolling message window,
// number formatting, encoded string decoding, key polling with the hot key hint overlay).
//
// All state lives in the data segment image so that every routine can be compared with the original byte for byte.
#include "wasteland1/machine.h"

namespace wl {

// ---- keyboard (BIOS int 16h model + WL.EXE's checkKeypress layer) ---------------------------------------------
bool biosKeyPending(Machine &m, uint16_t &key);   // int 16h AH=1: key = ah<<8 | al
uint16_t biosKeyRead(Machine &m);                 // int 16h AH=0 (waits through m.onIdle)
uint8_t checkKeypress(Machine &m, uint8_t caseDelta = 0x20);   // 18EFE / 18EF9: 0 when no key; F-key macros are handled here
bool waitForKeypress(Machine &m);                 // 18E90: waits for a key; true if it was ESC (carry set)
bool waitSpaceOrEnter(Machine &m);                // 18EAD: wait for SPACE (carry clear on ENTER, set on ESC/SPACE)
void hintOverlay(Machine &m);                     // 189B1: redraws the hot key hint labels for buttonMask1/2
void pollSpeedKeys(Machine &m);                   // 19004

// ---- numbers -----------------------------------------------------------------------------------------------------
void convertNumberToBcd(Machine &m);              // 18DCE: numberValue (24 bit) -> 8 digit packed BCD at numberBcd
void setNumberValue(Machine &m, uint32_t v);      // numberValue = v (3 bytes)
void writeNumber(Machine &m);                     // 17696: numberValue, up to 8 digits without padding
void writeNumberShort(Machine &m, uint8_t value); // 1769C: space padded to tempTextX columns
void writeNumberLeft(Machine &m, uint8_t value);  // 176A2
void writeNumber16(Machine &m, uint16_t value);   // 176D0
void writeSpaceNumber(Machine &m, uint16_t value);// 176C5

// ---- characters and strings --------------------------------------------------------------------------------------
void writeCharacter(Machine &m, uint8_t ch);      // 19E53: line buffered, word wrapped
void writeCharacter2(Machine &m, uint8_t ch);     // 19DC3: drawn immediately
void writeString(Machine &m);                     // 1786E: NUL terminated string at textString via the buffered writer
// 16CB7: printMessage without selecting the character writer first
void printMessageKeepWriter(Machine &m, uint8_t id);
void writeString2(Machine &m);                    // 17877: same, with the writer chosen by setWriteCharacterFn*
void setWriteCharacterFn(Machine &m);             // 1785E: buffered writer
void setWriteCharacterFn2(Machine &m);            // 17857: immediate writer
void writeItemText(Machine &m, uint8_t index);   // 17AD3: string `index` of the item text table (ITEMS_TEXT) through the current writer
void writeItemTextNormal(Machine &m, uint8_t index);   // 17ACE: same through the buffered writer (setWriteCharacterFn)
void writeItemName(Machine &m, uint8_t itemId);   // 17AF0: name of an item (strings start at index 36)
void writeCharacterVia(Machine &m, uint8_t ch);   // 1789C: writes through the current writer (writeCharacterFn)
void writeEncodedText(Machine &m, uint8_t index); // 178A3: decodes the 5-bit string `index` of textPointer1 and writes it
void printMessage(Machine &m, uint8_t id);        // 16CB2: string `id` of the common message table
void printMapMessage(Machine &m, uint8_t id);     // 178A0: string `id` of the current map
void writeMemberName(Machine &m);                 // 171B9
void saveRectBounds(Machine &m);                  // 19083
void restoreRectBounds(Machine &m);               // 19060
void drawBox(Machine &m, uint8_t right, uint8_t bottom);   // 19812
void drawInfoWindow(Machine &m);                  // 19727
void drawRosterWindow(Machine &m);                // 19770
void clearTextArea(Machine &m);                   // 197AE
void scrollLine(Machine &m);                      // 1A0C5
void scrollWindowUp(Machine &m);                  // 107D8
void eraseToRight(Machine &m);                    // 106C9
void newline(Machine &m);                         // 19EFC
void flushLine(Machine &m);                       // 19F12

} // namespace wl
